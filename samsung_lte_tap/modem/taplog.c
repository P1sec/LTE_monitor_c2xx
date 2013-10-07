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
#include <stdarg.h>
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#endif

#include "tapcfg.h"
#include "taplog.h"

void
taplog_init(taplog_t *taplog)
{
	assert(taplog);

	taplog->level = TAPLOG_INFO;
	taplog->callback = NULL;
}

void
taplog_set_level(taplog_t *taplog, int level)
{
	assert(taplog);

	taplog->level = level;
}

void
taplog_set_callback(taplog_t *taplog, taplog_callback_t callback)
{
	assert(taplog);

	taplog->callback = callback;
}

char *
taplog_utf8_to_local(const char *str)
{
	char *ret = NULL;

/* FIXME: This is only implemented on Windows for now */
#if defined(_WIN32) || defined(_WIN64)
	int wclen, mblen;
	WCHAR *wcstr;
	BOOL failed;

	wclen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	wcstr = malloc(sizeof(WCHAR) * wclen);
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wcstr, wclen);

	mblen = WideCharToMultiByte(CP_ACP, 0, wcstr, wclen, NULL, 0, NULL, &failed);
	if (failed) {
		/* Invalid characters in input, conversion failed */
		free(wcstr);
		return NULL;
	}

	ret = malloc(sizeof(CHAR) * mblen);
	WideCharToMultiByte(CP_ACP, 0, wcstr, wclen, ret, mblen, NULL, NULL);
	free(wcstr);
#endif

	return ret;
}

void
taplog_log(taplog_t *taplog, int level, const char *fmt, ...)
{
	char buffer[4096];
	va_list ap;

	if (level > taplog->level)
		return;

	buffer[sizeof(buffer)-1] = '\0';
	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer)-1, fmt, ap);
	va_end(ap);

	if (taplog->callback) {
		taplog->callback(level, buffer);
	} else {
		char *local = taplog_utf8_to_local(buffer);

		if (local) {
			fprintf(stderr, "%s\n", local);
			free(local);
		} else {
			fprintf(stderr, "%s\n", buffer);
		}
	}
}

void
taplog_log_ethernet_info(taplog_t *taplog, int level, unsigned char *buffer, int len) {
	assert(taplog);

	if (len < 14)
		return;

	taplog_log(taplog, level,
	           "Frame length %d (0x%04x) bytes",
	           len, len);
	taplog_log(taplog, level,
	           "Ethernet src address: %02x:%02x:%02x:%02x:%02x:%02x",
	           (buffer[6])&0xff, (buffer[7])&0xff, (buffer[8])&0xff, (buffer[9])&0xff,
	           (buffer[10])&0xff, (buffer[11])&0xff);
	taplog_log(taplog, level,
	           "Ethernet dst address: %02x:%02x:%02x:%02x:%02x:%02x",
	           (buffer[0])&0xff, (buffer[1])&0xff, (buffer[2])&0xff, (buffer[3])&0xff,
	           (buffer[4])&0xff, (buffer[5])&0xff);
	taplog_log(taplog, level,
	           "EtherType 0x%04x",
	           ((buffer[12] << 8) | buffer[13])&0xffff);
}
