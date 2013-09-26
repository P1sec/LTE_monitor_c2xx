/*
 * Copyright (C) 2010 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>

#include "usbdump.h"


#define SYSBASE			"/sys/bus/usb/devices"
#define USBMON_DEVICE	"/dev/usbmon"
#define MAX_PACKETS		32

#define USB_DIR_IN		0x80
#define LINEBUF_LEN		16383

int opt_unique_num = 0;

int64_t start_ts = 0;
int32_t start_ts_us = 0;
char **lines = {0};


char *pretty_xfertype[] = {
	" iso",
	"intr",
	"ctrl",
	"bulk"
};


void hexdump(char *linebuf, void *address, int length)
{
	int n, i;
	unsigned char *buf;

	n = strlen(linebuf);
	buf = address;
	for (i = 0; i < length; i++) {
		if (n > LINEBUF_LEN - 4)
			continue;
		if (i && !(i & 0x01)) {
			linebuf[n++] = ' ';
			linebuf[n] = '\0';
		}
		if (i && !(i & 0x0f)) {
			linebuf[n++] = ' ';
			linebuf[n] = '\0';
		}
		snprintf(linebuf+n, LINEBUF_LEN-n, "%.2x", buf[i]);
		n += 2;
	}
}


void process_packet(struct usbmon_packet *hdr, char *data)
{
	static int unique_ready = 0, unique_cnt = 0, unique_setcnt = 0, cnt = 0;
	int i;
	int64_t ts;
	int32_t ts_us;
	char *linebuf;

	/* basic direction filtering, but not for control packets */
	if (hdr->xfer_type != XFER_TYPE_CONTROL) {
		if (hdr->epnum & USB_DIR_IN) {
			if (hdr->type == 'S')
				/* read request */
				return;
		} else {
			if (hdr->type == 'C')
				/* ack to a write packet */
				return;
		}
	}

	if (!start_ts && !start_ts_us) {
		/* first packet */
		start_ts = hdr->ts_sec;
		start_ts_us = hdr->ts_usec;
	}

	linebuf = malloc(LINEBUF_LEN+1);
	linebuf[0] = '\0';

	ts = hdr->ts_sec - start_ts;
	ts_us = hdr->ts_usec - start_ts_us;
	if (ts_us < 0) {
		ts -= 1;
		ts_us = 1000000 + ts_us;
	}

	if (hdr->epnum & USB_DIR_IN)
		snprintf(linebuf+strlen(linebuf), LINEBUF_LEN-strlen(linebuf), "%d<-- ", hdr->epnum & 0x7f);
	else
		snprintf(linebuf+strlen(linebuf), LINEBUF_LEN-strlen(linebuf), "-->%d ", hdr->epnum & 0x7f);

	if (hdr->len_cap > 0) {
		if (hdr->len_cap == hdr->length)
			snprintf(linebuf+strlen(linebuf), LINEBUF_LEN-strlen(linebuf), "%d: ", hdr->len_cap);
		else
			snprintf(linebuf+strlen(linebuf), LINEBUF_LEN-strlen(linebuf), "%d/%d: ", hdr->len_cap, hdr->length);
		hexdump(linebuf, data, hdr->len_cap);
	}

	if (opt_unique_num) {
		if (!unique_ready) {
			lines[unique_cnt] = strdup(linebuf);
			printf("%3"PRId64".%.6d %s\n", ts, ts_us, linebuf);
		}
		else if (strcmp(linebuf, lines[unique_cnt])) {
			/* line is not the same as the previous one in the sequence */
			/* show the entire sequence leading up to this line */
			if (unique_setcnt > 1)
				printf("\n");
			for (i = 0; i < unique_cnt; i++) {
				printf("%3"PRId64".%.6d %s\n", ts, ts_us, lines[i]);
				free(lines[i]);
			}

			printf("%3"PRId64".%.6d %s\n", ts, ts_us, linebuf);

			/* this line starts a new sequence */
			lines[0] = strdup(linebuf);
			unique_cnt = 0;
			unique_setcnt = 0;
			unique_ready = 0;
		}

		if (++unique_cnt == opt_unique_num) {
			if (unique_ready) {
				printf("last %d lines repeated %d times\r", opt_unique_num, unique_setcnt);
				fflush(stdout);
			}
			unique_ready = 1;
			unique_cnt = 0;
			unique_setcnt++;
		}
	}
	else
		printf("%3"PRId64".%.6d %s\n", ts, ts_us, linebuf);

	free(linebuf);

}


void usb_sniff(bus, address)
{
	struct mon_mfetch_arg mfetch;
	struct usbmon_packet *hdr;
	int kbuf_len, fd, nflush, i;
	char *mbuf, path[64];
	uint32_t vec[MAX_PACKETS];

	snprintf(path, 63, "%s%d", USBMON_DEVICE, bus);
	if ( (fd = open(path, O_RDONLY)) == -1 ) {
		printf("unable to open %s: %s\n", path, strerror(errno));
		return;
	}

	if ( (kbuf_len = ioctl(fd, MON_IOCQ_RING_SIZE)) <= 0) {
		printf("failed to determine kernel USB buffer size: %s\n", strerror(errno));
		return;
	}

	if ( (mbuf = mmap(NULL, kbuf_len, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		printf("unable to mmap %d bytes: %s\n", kbuf_len, strerror(errno));
		return;
	}

	if (opt_unique_num)
		lines = malloc(opt_unique_num * sizeof(char *));
	nflush = 0;
	while (1) {
		mfetch.offvec = vec;
		mfetch.nfetch = MAX_PACKETS;
		mfetch.nflush = nflush;
		ioctl(fd, MON_IOCX_MFETCH, &mfetch);
		nflush = mfetch.nfetch;
		for (i = 0; i < mfetch.nfetch; i++) {
			hdr = (struct usbmon_packet *) &mbuf[vec[i]];
			if (hdr->type == '@')
				/* filler packet */
				continue;
			if (hdr->busnum != bus || hdr->devnum != address)
				/* some other device */
				continue;

			process_packet(hdr, &mbuf[vec[i]]+sizeof(struct usbmon_packet));
		}
	}
	free(lines);
	munmap(mbuf, kbuf_len);
	ioctl(fd, MON_IOCH_MFLUSH, mfetch.nfetch);
	close(fd);

}


int check_device(struct dirent *de, char *vidpid, int *bus, int *address)
{
	int fd;
	char path[128], vid[5], buf[5];

	/* vendor */
	memcpy(vid, vidpid, 4);
	vid[4] = '\0';
	sprintf(path, "%s/%s/idVendor", SYSBASE, de->d_name);
	if ( (fd = open(path, O_RDONLY)) == -1 )
		return 0;
	buf[4] = '\0';
	if ( (read(fd, buf, 4) != 4) || strncmp(vid, buf, 4) ) {
		close(fd);
		return 0;
	}

	/* product */
	sprintf(path, "%s/%s/idProduct", SYSBASE, de->d_name);
	if ( (fd = open(path, O_RDONLY)) == -1 )
		return 0;
	buf[4] = '\0';
	if ( (read(fd, buf, 4) != 4) || strncmp(vidpid+5, buf, 4) ) {
		close(fd);
		return 0;
	}

	/* bus */
	sprintf(path, "%s/%s/busnum", SYSBASE, de->d_name);
	if ( (fd = open(path, O_RDONLY)) == -1 )
		return 0;
	memset(buf, 0, 5);
	read(fd, buf, 4);
	*bus = strtol(buf, NULL, 10);

	/* address */
	sprintf(path, "%s/%s/devnum", SYSBASE, de->d_name);
	if ( (fd = open(path, O_RDONLY)) == -1 )
		return 0;
	memset(buf, 0, 5);
	read(fd, buf, 4);
	*address = strtol(buf, NULL, 10);

	return 1;
}


int find_device(char *vidpid, int *bus, int *address)
{
	DIR *dir;
	struct dirent *de;
	char buf[16];
	int found;

	if ( !(dir = opendir(SYSBASE)) )
		return 0;

	found = 0;
	while ( (de = readdir(dir)) ) {
		if (check_device(de, vidpid, bus, address)) {
			found = 1;
			break;
		}
	}
	closedir(dir);

	return found;
}


void usage(void)
{

	printf("usbdump Copyright (C) 2011 Bert Vermeulen <bert@biot.com>\n");
	printf("usage: usbdump -d <vid:pid> [-u <num lines>]\n");

}

int main(int argc, char **argv)
{
	int opt, bus, address;
	char *device, *entry;

	device = NULL;
	while ((opt = getopt(argc, argv, "d:u:")) != -1) {
		switch (opt) {
		case 'd':
			if (strlen(optarg) != 9 || strspn(optarg, "01234567890abcdef:") != 9)
				printf("invalid format for device id (aaaa:bbbb)\n");
			else
				device = optarg;
			break;
		case 'u':
			opt_unique_num = strtol(optarg, NULL, 10);
			break;
		}
	}

	if (!device) {
		usage();
		return 1;
	}

	if (find_device(device, &bus, &address)) {
		usb_sniff(bus, address);
	}

	return 0;
}



