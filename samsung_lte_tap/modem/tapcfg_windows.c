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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <winioctl.h>
#include <ws2tcpip.h>

#include "tapcfg.h"
#include "taplog.h"

#include "tapcfg_windows_fixup.h"

#define TAP_CONTROL_CODE(request,method)  CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
#define TAP_IOCTL_GET_MAC                 TAP_CONTROL_CODE(1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION             TAP_CONTROL_CODE(2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU                 TAP_CONTROL_CODE(3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO                TAP_CONTROL_CODE(4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT   TAP_CONTROL_CODE(5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS        TAP_CONTROL_CODE(6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ        TAP_CONTROL_CODE(7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE            TAP_CONTROL_CODE(8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT     TAP_CONTROL_CODE(9, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_TUN              TAP_CONTROL_CODE(10, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCPV6_MASQ      TAP_CONTROL_CODE(11, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCPV6_SET_OPT   TAP_CONTROL_CODE(12, METHOD_BUFFERED)

#define TAP_DEVICE_DIR	                  "\\\\.\\Global\\"

#define TAP_WINDOWS_MIN_MAJOR               9
#define TAP_WINDOWS_MIN_MINOR               8

typedef unsigned char MACADDR [6];
typedef unsigned long IPADDR;

struct tapcfg_s {
	TAPCFG_COMMON;

	HANDLE dev_handle;
	char *ifname;
	MACADDR hwaddr;

	int reading;
	OVERLAPPED overlapped_in;
	OVERLAPPED overlapped_out;

	char inbuf[TAPCFG_BUFSIZE];
	DWORD inbuflen;
};

tapcfg_t *
tapcfg_init()
{
	tapcfg_t *tapcfg;

	tapcfg = calloc(1, sizeof(tapcfg_t));
	if (!tapcfg) {
		return NULL;
	}

	taplog_init(&tapcfg->taplog);

	tapcfg->dev_handle = INVALID_HANDLE_VALUE;
	tapcfg->overlapped_in.hEvent =
		CreateEvent(NULL, FALSE, FALSE, NULL);
	tapcfg->overlapped_out.hEvent =
		CreateEvent(NULL, FALSE, FALSE, NULL);

	return tapcfg;
}

void
tapcfg_destroy(tapcfg_t *tapcfg)
{
	if (tapcfg) {
		tapcfg_stop(tapcfg);

		free(tapcfg->ifname);

		CloseHandle(tapcfg->overlapped_in.hEvent);
		CloseHandle(tapcfg->overlapped_out.hEvent);
	}
	free(tapcfg);
}

int
tapcfg_start(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	char *adapterid = NULL;
	char tapname[1024];
	HANDLE dev_handle;
	DWORD len;

	assert(tapcfg);

	if (!ifname) {
		ifname = "";
		fallback = 1;
	}

	tapcfg->ifname = tapcfg_fixup_adapters(&tapcfg->taplog, ifname, &adapterid, fallback);
	if (!tapcfg->ifname) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "TAP adapter not configured properly...");
		return -1;
	}

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "TAP adapter configured properly");
	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Interface name is '%s'", tapcfg->ifname);

	tapname[sizeof(tapname)-1] = '\0';
	snprintf(tapname, sizeof(tapname)-1, TAP_DEVICE_DIR "%s.tap", adapterid);
	free(adapterid);

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Trying %s", tapname);
	dev_handle = CreateFile(tapname,
				GENERIC_WRITE | GENERIC_READ,
				0, /* ShareMode, don't let others open the device */
				0, /* SecurityAttributes */
				OPEN_EXISTING,
				FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
				0); /* TemplateFile */

	if (dev_handle != INVALID_HANDLE_VALUE) {
		unsigned long info[3];

		if (!DeviceIoControl(dev_handle,
				     TAP_IOCTL_GET_VERSION,
				     &info, /* InBuffer */
				     sizeof(info),
				     &info, /* OutBuffer */
				     sizeof(info),
				     &len, NULL)) {

			taplog_log(&tapcfg->taplog, TAPLOG_ERR,
				   "Error calling DeviceIoControl: %d",
				   (int) GetLastError());
			CloseHandle(dev_handle);
			dev_handle = INVALID_HANDLE_VALUE;
		} else {
			taplog_log(&tapcfg->taplog, TAPLOG_DEBUG,
				   "TAP Driver Version %d.%d %s",
				   (int) info[0],
				   (int) info[1],
				   info[2] ? "(DEBUG)" : "");

			if (info[0] < TAP_WINDOWS_MIN_MAJOR ||
			    (info[0] == TAP_WINDOWS_MIN_MAJOR && info[1] < TAP_WINDOWS_MIN_MINOR)) {
				taplog_log(&tapcfg->taplog, TAPLOG_ERR,
					   "A TAP driver is required that is at least version %d.%d",
					   TAP_WINDOWS_MIN_MAJOR, TAP_WINDOWS_MIN_MINOR);
				taplog_log(&tapcfg->taplog, TAPLOG_INFO,
					   "If you recently upgraded your TAP driver, a reboot is probably "
					   "required at this point to get Windows to see the new driver.");

				CloseHandle(dev_handle);
				dev_handle = INVALID_HANDLE_VALUE;
			}
		}
	}

	if (dev_handle != INVALID_HANDLE_VALUE) {
		MACADDR hwaddr;

		if (!DeviceIoControl(dev_handle,
				     TAP_IOCTL_GET_MAC,
				     &hwaddr, /* InBuffer */
				     sizeof(hwaddr),
				     &hwaddr, /* OutBuffer */
				     sizeof(hwaddr),
				     &len, NULL)) {

			taplog_log(&tapcfg->taplog, TAPLOG_ERR,
				   "Error calling DeviceIoControl: %d",
				   (int) GetLastError());
			CloseHandle(dev_handle);
			dev_handle = INVALID_HANDLE_VALUE;

			return -1;
		}

		taplog_log(&tapcfg->taplog, TAPLOG_DEBUG,
			   "TAP interface MAC address %.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
		           hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

		memcpy(tapcfg->hwaddr, hwaddr, sizeof(hwaddr));
	}

	if (dev_handle == INVALID_HANDLE_VALUE) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "No working Tap device found!");
		return -1;
	}

	tapcfg->dev_handle = dev_handle;
	tapcfg->started = 1;
	tapcfg->status = 0;

	return 0;
}

void
tapcfg_stop(tapcfg_t *tapcfg)
{
	assert(tapcfg);

	if (tapcfg->started) {
		if (tapcfg->dev_handle != INVALID_HANDLE_VALUE) {
			CloseHandle(tapcfg->dev_handle);
			tapcfg->dev_handle = INVALID_HANDLE_VALUE;
		}
		tapcfg->started = 0;
		tapcfg->status = 0;
	}
}

int
tapcfg_get_fd(tapcfg_t *tapcfg)
{
  return _open_osfhandle(tapcfg->dev_handle, _O_APPEND);
}

static int
tapcfg_wait_for_data(tapcfg_t *tapcfg, DWORD timeout)
{
	DWORD retval, len;
	int ret = 0;

	assert(tapcfg);

	if (tapcfg->inbuflen) {
		taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Found %d bytes from buffer", tapcfg->inbuflen);
		return 1;
	} else if (!tapcfg->reading) {
		/* No data available, start a new read */
		tapcfg->reading = 1;

		tapcfg->overlapped_in.Offset = 0;
		tapcfg->overlapped_in.OffsetHigh = 0;

		taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling ReadFile function");
		retval = ReadFile(tapcfg->dev_handle,
		                  tapcfg->inbuf,
		                  sizeof(tapcfg->inbuf),
		                  &len,
		                  &tapcfg->overlapped_in);

		/* If read successful, mark reading finished */
		if (retval) {
			tapcfg->reading = 0;
			tapcfg->inbuflen = len;
			ret = 1;
			taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Finished reading %d bytes with ReadFile", len);
		} else if (GetLastError() != ERROR_IO_PENDING) {
			tapcfg->reading = 0;
			taplog_log(&tapcfg->taplog, TAPLOG_ERR,
			           "Error calling ReadFile: %d",
			           GetLastError());
		}
	}

	if (tapcfg->reading) {
		retval = WaitForSingleObject(tapcfg->overlapped_in.hEvent, timeout);

		if (retval == WAIT_OBJECT_0) {
			taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling GetOverlappedResult function");
			retval = GetOverlappedResult(tapcfg->dev_handle,
			                             &tapcfg->overlapped_in,
			                             &len, FALSE);
			if (!retval) {
				tapcfg->reading = 0;
				taplog_log(&tapcfg->taplog, TAPLOG_ERR,
				           "Error calling GetOverlappedResult: %d",
				           GetLastError());
			} else {
				tapcfg->reading = 0;
				tapcfg->inbuflen = len;
				ret = 1;
				taplog_log(&tapcfg->taplog, TAPLOG_DEBUG,
				           "Finished reading %d bytes with GetOverlappedResult",
				           len);
			}
		}
	}

	return ret;
}

int
tapcfg_wait_readable(tapcfg_t *tapcfg, int msec)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	return tapcfg_wait_for_data(tapcfg, msec);
}

int
tapcfg_read(tapcfg_t *tapcfg, void *buf, int count)
{
	int ret;

	assert(tapcfg);

	if (!tapcfg->started) {
		return -1;
	}

	if (!tapcfg_wait_for_data(tapcfg, INFINITE)) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error waiting for data in read function");
		return -1;
	}

	if (count < tapcfg->inbuflen) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Buffer not big enough for reading, "
		           "need at least %d bytes",
		           tapcfg->inbuflen);
		return -1;
	}

	ret = tapcfg->inbuflen;
	memcpy(buf, tapcfg->inbuf, tapcfg->inbuflen);
	tapcfg->inbuflen = 0;

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Read ethernet frame:");
	taplog_log_ethernet_info(&tapcfg->taplog, TAPLOG_DEBUG, buf, ret);

	return ret;
}

int
tapcfg_wait_writable(tapcfg_t *tapcfg, int msec)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	return 1;
}

int
tapcfg_write(tapcfg_t *tapcfg, void *buf, int count)
{
	DWORD retval, len;

	assert(tapcfg);

	if (!tapcfg->started) {
		return -1;
	}

	tapcfg->overlapped_out.Offset = 0;
	tapcfg->overlapped_out.OffsetHigh = 0;

	retval = WriteFile(tapcfg->dev_handle, buf, count,
	                   &len, &tapcfg->overlapped_out);
	if (!retval && GetLastError() == ERROR_IO_PENDING) {
		retval = WaitForSingleObject(tapcfg->overlapped_out.hEvent,
		                             INFINITE);
		if (!retval) {
			taplog_log(&tapcfg->taplog, TAPLOG_ERR,
			           "Error calling WaitForSingleObject");
			return -1;
		}
		retval = GetOverlappedResult(tapcfg->dev_handle,
		                             &tapcfg->overlapped_out,
		                             &len, FALSE);
	}

	if (!retval) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR,
		           "Error trying to write data to TAP device: %d",
		           GetLastError());
		return -1;
	}

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Wrote ethernet frame:");
	taplog_log_ethernet_info(&tapcfg->taplog, TAPLOG_DEBUG, buf, len);

	return len;
}

char *
tapcfg_get_ifname(tapcfg_t *tapcfg)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return NULL;
	}

	return tapcfg->ifname;
}

const char *
tapcfg_iface_get_hwaddr(tapcfg_t *tapcfg, int *length)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return NULL;
	}

	if (length)
		*length = sizeof(tapcfg->hwaddr);
	return (const char *) tapcfg->hwaddr;
}

int
tapcfg_iface_set_hwaddr(tapcfg_t *tapcfg, const char *hwaddr, int length)
{
	assert(tapcfg);

	if (!tapcfg->started || tapcfg->status) {
		return -1;
	}

	if (length != sizeof(tapcfg->hwaddr)) {
		return -1;
	}

	return -1;
}

int
tapcfg_iface_get_status(tapcfg_t *tapcfg)
{
	assert(tapcfg);

	return tapcfg->status;
}

int
tapcfg_iface_set_status(tapcfg_t *tapcfg, int flags)
{
	unsigned long status = flags;
	DWORD len;

	assert(tapcfg);

	if (!tapcfg->started) {
		return -1;
	} else if (flags == tapcfg->status) {
		/* Already enabled, nothing required */
		return 0;
	}

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling DeviceIoControl");
	if (!DeviceIoControl(tapcfg->dev_handle,
	                     TAP_IOCTL_SET_MEDIA_STATUS,
	                     &status, /* InBuffer */
	                     sizeof(status),
	                     &status, /* OutBuffer */
	                     sizeof(status),
	                     &len, NULL)) {
		return -1;
	}
	tapcfg->status = flags;

	return 0;
}

int
tapcfg_iface_get_mtu(tapcfg_t *tapcfg)
{
	ULONG mtu;
	DWORD len;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling DeviceIoControl for getting MTU");
	if (!DeviceIoControl(tapcfg->dev_handle,
	                     TAP_IOCTL_GET_MTU,
	                     &mtu, /* InBuffer */
	                     sizeof(mtu),
	                     &mtu, /* OutBuffer */
	                     sizeof(mtu),
	                     &len, NULL)) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Calling DeviceIoControl failed");
		return -1;
	}

	return mtu;
}

int
tapcfg_iface_set_mtu(tapcfg_t *tapcfg, int mtu)
{
	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	if (mtu < 68 || mtu > (TAPCFG_BUFSIZE - 22)) {
		return -1;
	}

	return -1;
}

int
tapcfg_iface_set_ipv4(tapcfg_t *tapcfg, const char *addrstr, unsigned char netbits)
{
	IPADDR buffer[4];
	IPADDR addr, mask;
	DWORD len;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	if (netbits == 0 || netbits > 32) {
		return -1;
	}

	/* Calculate the netmask from the network bit length */
	for (mask=0; netbits; netbits--)
		mask = (mask >> 1)|(1 << 31);

	/* Check that the given IPv4 address is valid */
	addr = inet_addr(addrstr);
	if (addr == INADDR_NONE || addr == INADDR_ANY)
		return -1;

	buffer[0] = addr;
	buffer[1] = htonl(mask);
	buffer[2] = htonl(htonl(buffer[0] | ~buffer[1])-1);
	buffer[3] = 3600;

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling DeviceIoControl for MASQ");
	if (!DeviceIoControl(tapcfg->dev_handle,
	                     TAP_IOCTL_CONFIG_DHCP_MASQ,
	                     &buffer, /* InBuffer */
	                     sizeof(buffer),
	                     &buffer, /* OutBuffer */
	                     sizeof(buffer),
	                     &len, NULL)) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Calling DeviceIoControl failed");
		return -1;
	}

	return 0;
}

int
tapcfg_iface_set_ipv6(tapcfg_t *tapcfg, const char *addrstr, unsigned char netbits)
{
#ifdef HAVE_GETADDRINFO
	UCHAR buffer[7*sizeof(ULONG)];
	ULONG *lbuffer = (ULONG *)buffer;

	struct addrinfo hints;
	struct addrinfo *result = NULL;
	struct sockaddr_in6 *sockaddr_ipv6;

	DWORD len, res;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	if (netbits == 0 || netbits > 128) {
		return -1;
	}

	/* Setup the hints structure */
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_INET6;

	/* Check that the given IPv6 address is valid */
	res = getaddrinfo(addrstr, NULL, &hints, &result);
	if (res != 0 || result == NULL) {
		return -1;
	}

	/* Copy the IPv6 address into the buffer */
	sockaddr_ipv6 = (struct sockaddr_in6 *)result->ai_addr;
	memcpy(buffer, &sockaddr_ipv6->sin6_addr, sizeof(struct in6_addr));
	sockaddr_ipv6 = NULL;
	freeaddrinfo(result);
	result = NULL;

	/* Initialize the other variables in buffer */
	lbuffer[4] = netbits;
	lbuffer[5] = 0;
	lbuffer[6] = 3600;

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling DeviceIoControl for IPv6 MASQ");
	if (!DeviceIoControl(tapcfg->dev_handle,
	                     TAP_IOCTL_CONFIG_DHCPV6_MASQ,
	                     &buffer, /* InBuffer */
	                     sizeof(buffer),
	                     &buffer, /* OutBuffer */
	                     sizeof(buffer),
	                     &len, NULL)) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Calling DeviceIoControl failed");
		return -1;
	}

	return 0;
#else
	return -1;
#endif
}

int
tapcfg_iface_set_dhcp_options(tapcfg_t *tapcfg, unsigned char *buffer, int buflen)
{
	DWORD len;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling DeviceIoControl for DHCP_SET_OPT\n");
	if (!DeviceIoControl(tapcfg->dev_handle,
	                     TAP_IOCTL_CONFIG_DHCP_SET_OPT,
	                     buffer, /* InBuffer */
	                     buflen,
	                     buffer, /* OutBuffer */
	                     buflen,
	                     &len, NULL)) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Calling DeviceIoControl failed\n");
		return -1;
	}

	return 0;
}

int
tapcfg_iface_set_dhcpv6_options(tapcfg_t *tapcfg, unsigned char *buffer, int buflen)
{
	DWORD len;

	assert(tapcfg);

	if (!tapcfg->started) {
		return 0;
	}

	taplog_log(&tapcfg->taplog, TAPLOG_DEBUG, "Calling DeviceIoControl for DHCPV6_SET_OPT\n");
	if (!DeviceIoControl(tapcfg->dev_handle,
	                     TAP_IOCTL_CONFIG_DHCPV6_SET_OPT,
	                     buffer, /* InBuffer */
	                     buflen,
	                     buffer, /* OutBuffer */
	                     buflen,
	                     &len, NULL)) {
		taplog_log(&tapcfg->taplog, TAPLOG_ERR, "Calling DeviceIoControl failed\n");
		return -1;
	}

	return 0;
}
