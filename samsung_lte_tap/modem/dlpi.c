/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2011  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

/* This file includes the useful DLPI functions we need on Solaris
 * to get and set the hardware address. For more information about
 * DLPI on Solaris, there's a great website available at the address
 * http://www.mm.kay-mueller.de/solaris_drivers.html. Without it
 * writing these functions would've been much more painful */


#ifdef __sun__

#include <string.h>
#include <stropts.h>
#include <sys/dlpi.h>

static int
dlpi_put_msg(int fd,
              void *prim, int prim_len,
              void *data, int data_len,
              int flags)
{
	struct strbuf ctrlbuf, databuf;
	int ret;

	ctrlbuf.buf = (char *) prim;
	ctrlbuf.len = prim_len;

	databuf.buf = (char *) data;
	databuf.len = data_len;

	ret = putmsg(fd, &ctrlbuf, (data ? &databuf : NULL), flags);

	return ret;
}

static int
dlpi_get_msg(int fd,
             void *prim, int prim_len,
             void *data, int *data_len,
             int *flags)
{
	struct strbuf ctrlbuf, databuf;
	int tmpflags, ret;

	ctrlbuf.buf = (char *) prim;
	ctrlbuf.maxlen = prim_len;

	databuf.buf = (char *) data;
	databuf.maxlen = (data_len ? *data_len : 0);

	tmpflags = (flags ? *flags : 0);

	ret = getmsg(fd, &ctrlbuf, &databuf, &tmpflags);

	if (data_len)
		*data_len = databuf.len;
	if (flags)
		*flags = tmpflags;

	return ret;
}


#define DLPIBUFSIZE 512


int
dlpi_attach(int fd, int ppa)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_attach_req_t dl_attach_req;
	dl_ok_ack_t *p_ok_ack;
	int ret;

	memset(&dl_attach_req, 0, sizeof(dl_attach_req));
	dl_attach_req.dl_primitive = DL_ATTACH_REQ;
	dl_attach_req.dl_ppa = ppa;
	ret = dlpi_put_msg(fd, &dl_attach_req,
	                   sizeof(dl_attach_req),
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_ok_ack = (dl_ok_ack_t *) buffer;
	if (p_ok_ack->dl_primitive != DL_OK_ACK) {
		return -1;
	}

	return 0;
}

int
dlpi_detach(int fd)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_detach_req_t dl_detach_req;
	dl_ok_ack_t *p_ok_ack;
	int ret;

	memset(&dl_detach_req, 0, sizeof(dl_detach_req));
	dl_detach_req.dl_primitive = DL_DETACH_REQ;
	ret = dlpi_put_msg(fd, &dl_detach_req,
	                   sizeof(dl_detach_req),
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_ok_ack = (dl_ok_ack_t *) buffer;
	if (p_ok_ack->dl_primitive != DL_OK_ACK) {
		return -1;
	}

	return 0;
}

int
dlpi_get_physaddr(int fd, unsigned char *hwaddr, int length)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_phys_addr_req_t dl_phys_addr_req;
	dl_phys_addr_ack_t *p_phys_addr_ack;
	void *result;
	int ret;

	memset(&dl_phys_addr_req, 0, sizeof(dl_phys_addr_req));
	dl_phys_addr_req.dl_primitive = DL_PHYS_ADDR_REQ;
	dl_phys_addr_req.dl_addr_type = DL_CURR_PHYS_ADDR;
	ret = dlpi_put_msg(fd, &dl_phys_addr_req,
	                   sizeof(dl_phys_addr_req),
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_phys_addr_ack = (dl_phys_addr_ack_t *) buffer;
	if (p_phys_addr_ack->dl_primitive != DL_PHYS_ADDR_ACK) {
		return -1;
	}

	if (p_phys_addr_ack->dl_addr_length > length) {
		return -1;
	}
	length = p_phys_addr_ack->dl_addr_length;

	result = ((char *) p_phys_addr_ack) +
	         p_phys_addr_ack->dl_addr_offset;
	memcpy(hwaddr, result, length);

	return length;
}

int
dlpi_set_physaddr(int fd, const char *hwaddr, int length)
{
	unsigned char buffer[DLPIBUFSIZE];
	dl_set_phys_addr_req_t *p_set_phys_addr_req;
	dl_ok_ack_t *p_ok_ack;
	int offset, ret;

	p_set_phys_addr_req = (dl_set_phys_addr_req_t *) buffer;
	memset(p_set_phys_addr_req, 0, sizeof(dl_set_phys_addr_req_t));
	p_set_phys_addr_req->dl_primitive = DL_SET_PHYS_ADDR_REQ;
	p_set_phys_addr_req->dl_addr_length = length;

	offset = sizeof(dl_set_phys_addr_req_t);
	memcpy(buffer + offset, hwaddr, length);
	p_set_phys_addr_req->dl_addr_offset = offset;
	
	ret = dlpi_put_msg(fd, p_set_phys_addr_req,
	                   sizeof(dl_set_phys_addr_req_t) + length,
	                   NULL, 0, 0);
	if (ret < 0) {
		return -1;
	}

	ret = dlpi_get_msg(fd, buffer, sizeof(buffer),
	                   NULL, NULL, 0);
	if (ret != 0) {
		return -1;
	}
	p_ok_ack = (dl_ok_ack_t *) buffer;
	if (p_ok_ack->dl_primitive != DL_OK_ACK) {
		return -1;
	}

	return 0;
}

#else

int
dlpi_attach(int fd, int ppa)
{
	return -1;
}

int
dlpi_detach(int fd)
{
	return -1;
}

int
dlpi_get_physaddr(int fd, unsigned char *hwaddr, int length)
{
	return -1;
}

int
dlpi_set_physaddr(int fd, const char *hwaddr, int length)
{
	return -1;
}

#endif

#ifdef MAIN
#include <stdio.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
	int fd, ppa;
	const char *endptr;
	unsigned char hwaddr[6];
	unsigned char testaddr[6] = { 0x00, 0x01, 0x23, 0x45, 0x67, 0x89 };

	if (argc < 3) {
		printf("Not enough arguments!\n");
		printf("Usage: %s (device) (ppa)\n", argv[0]);
		printf("Example for pcn0: %s /dev/pcn 0\n", argv[0]);
		return -1;
	}

	ppa = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		printf("Argument %s not a valid integer\n", argv[2]);
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error opening device %s", argv[1]);
		return -1;
	}

	dlpi_attach(fd, ppa);
	dlpi_get_physaddr(fd, hwaddr, sizeof(hwaddr));
	printf("Got physical address: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
	       hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	dlpi_set_physaddr(fd, (const char *) testaddr, sizeof(testaddr));
	memcpy(testaddr, hwaddr, sizeof(hwaddr));
	dlpi_get_physaddr(fd, hwaddr, sizeof(hwaddr));
	printf("Got physical address after set: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
	       hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	dlpi_set_physaddr(fd, (const char *) testaddr, sizeof(testaddr));
	dlpi_get_physaddr(fd, hwaddr, sizeof(hwaddr));
	printf("Got physical address after second set: %.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",
	       hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	dlpi_detach(fd);
	close(fd);

	return 0;
}

#endif

