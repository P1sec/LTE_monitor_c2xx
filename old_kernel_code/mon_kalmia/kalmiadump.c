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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gsmtap.h"


#define C2XX_DEVICE	"/dev/c2xx"
#define MAX_PACKETS		32

struct c2xx_mfetch_arg {
    uint32_t nfetch;		/* Number of events to fetch (out: fetched) */
	uint32_t nlength;		/* Number of events to flush */
	unsigned char *packet;
};

#define C2XX_IOC_MAGIC		0x92
#define C2XX_IOCX_MFETCH		_IOWR(C2XX_IOC_MAGIC, 7, struct c2xx_mfetch_arg)

int hexprintf(unsigned char *c, unsigned int len) {

	int i,j;
	char tmp[16];

	while(len){
		for(i = 0; i < 16; i++){
			len--;
			printf("%02x ",*c);
			tmp[i] = *c;
			c++;
			/*if(i == 7){
			printf("  ");
			}*/

			if(!len){
				break;
			}
		}

		for(j = 0; j < 15 -i; j++){
			printf("   ");
		}

		printf("  |");
		for(j = 0; j < i; j++){
			if((tmp[j] > 32) && (tmp[j] < 128)){
				printf("%c",tmp[j]);
			} else {
				printf(".");
			}
		}
		for(j = 0; j < 16-i; j++){
			printf(" ");
		}

		printf("|\n");
	}
}

static int gsmtap_fd;

static void gsmtap_open(const char *gsmtap_host)
{
	struct sockaddr_in sin;

	sin.sin_family= AF_INET;
	sin.sin_port = htons(GSMTAP_UDP_PORT);

	if (inet_aton(gsmtap_host, &sin.sin_addr) < 0)
	{
		perror("parsing GSMTAP destination address");
		exit(2);
	}
	gsmtap_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (gsmtap_fd < 0)
	{
		perror("GSMTAP socket initialization");
		exit(2);
	}
	if (connect(gsmtap_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("connecting GSMTAP socket");
		exit(2);
	}
}

void c2xx_sniff()
{
	int ret;
	int fd;
	struct c2xx_mfetch_arg mfetch;
	char *linebuf;

	struct gsmtap_hdr gh = { .version = GSMTAP_VERSION,
		.hdr_len = sizeof(struct gsmtap_hdr)/4,
		.type = GSMTAP_TYPE_C2XX,
		.sub_type = GSMTAP_CHANNEL_UNKNOWN,
		.arfcn = htons(GSMTAP_ARFCN_F_UPLINK) };

	mfetch.packet = malloc(1200);

	if ( (fd = open(C2XX_DEVICE, O_RDONLY)) == -1 ) {
		printf("unable to open %s: %s\n", C2XX_DEVICE, strerror(errno));
		return;
	}
	

	while (1) {
		ret = ioctl(fd, C2XX_IOCX_MFETCH, &mfetch);
		/*
		nflush = mfetch.nfetch;
		for (i = 0; i < mfetch.nfetch; i++) {
		}
		*/
		if(ret!=0) {
			// printf("ioctl %08X\n", ret);
		} else {
			if(mfetch.nfetch) {
				printf("fetched %d\n", mfetch.nfetch);
				//printf("%02x %02x %02x %02x\n", mfetch.packets[0], mfetch.packets[1], mfetch.packets[2], mfetch.packets[4]);
				printf("length %d\n", mfetch.nlength);
				hexprintf(mfetch.packet, mfetch.nlength);

				// send to gsmtap
				const size_t buf_len = sizeof(struct gsmtap_hdr) + mfetch.nlength;
				unsigned char buf[buf_len];

				memcpy(buf, &gh, sizeof(struct gsmtap_hdr));
				memcpy(buf+sizeof(struct gsmtap_hdr), mfetch.packet, mfetch.nlength);

				if (write(gsmtap_fd, buf, buf_len) < 0)
					perror("write gsmtap\n");
			}
		}

	}


}

int main(int argc, char **argv)
{

	gsmtap_open(argv[1]);

	c2xx_sniff();

	return 0;
}
