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

/* this was taken from usbmon.txt in the kernel documentation */
#define SETUP_LEN 8
struct usbmon_packet {
	uint64_t id;			/* 0: URB ID - from submission to callback */
	unsigned char type;		/* 8: Same as text; extensible. */
	unsigned char xfer_type;/* ISO (0), Intr, Control, Bulk (3) */
	unsigned char epnum;	/* Endpoint number and transfer direction */
	unsigned char devnum;	/* Device address */
	uint16_t busnum;		/* 12: Bus number */
	char flag_setup;		/* 14: Same as text */
	char flag_data;			/* 15: Same as text; Binary zero is OK. */
	int64_t ts_sec;			/* 16: gettimeofday */
	int32_t ts_usec;		/* 24: gettimeofday */
	int status;				/* 28: */
	unsigned int length;	/* 32: Length of data (submitted or actual) */
	unsigned int len_cap;	/* 36: Delivered length */
	union {					/* 40: */
		unsigned char setup[SETUP_LEN]; /* Only for Control S-type */
		struct iso_rec {	/* Only for ISO */
			int error_count;
			int numdesc;
		} iso;
	} s;
	int interval;			/* 48: Only for Interrupt and ISO */
	int start_frame;		/* 52: For ISO */
	unsigned int xfer_flags;/* 56: copy of URB's transfer_flags */
	unsigned int ndesc;		/* 60: Actual number of ISO descriptors */
};							/* 64 total length */

struct mon_mfetch_arg {
    uint32_t *offvec;		/* Vector of events fetched */
    uint32_t nfetch;		/* Number of events to fetch (out: fetched) */
    uint32_t nflush;		/* Number of events to flush */
};
#define MON_IOC_MAGIC		0x92
#define MON_IOCX_MFETCH		_IOWR(MON_IOC_MAGIC, 7, struct mon_mfetch_arg)
#define MON_IOCQ_URB_LEN	_IO(MON_IOC_MAGIC, 1)
#define MON_IOCQ_RING_SIZE	_IO(MON_IOC_MAGIC, 5)
#define MON_IOCH_MFLUSH		_IO(MON_IOC_MAGIC, 8)

#define XFER_TYPE_ISO		0
#define XFER_TYPE_INTERRUPT	1
#define XFER_TYPE_CONTROL	2
#define XFER_TYPE_BULK		3




