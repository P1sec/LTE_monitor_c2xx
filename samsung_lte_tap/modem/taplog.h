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

#ifndef TAPLOG_H
#define TAPLOG_H

struct taplog_s {
	int level;
	taplog_callback_t callback;
};
typedef struct taplog_s taplog_t;

void taplog_init(taplog_t *taplog);
void taplog_set_level(taplog_t *taplog, int level);
void taplog_set_callback(taplog_t *taplog, taplog_callback_t callback);
char *taplog_utf8_to_local(const char *str);

void taplog_log(taplog_t *taplog, int level, const char *fmt, ...);
void taplog_log_ethernet_info(taplog_t *taplog, int level, unsigned char *buffer, int len);

#endif /* TAPLOG_H */

