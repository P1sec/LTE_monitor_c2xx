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

#include <string.h>

#include "tapcfg.h"
#include "taplog.h"

#define TAPCFG_BUFSIZE 4096

#define TAPCFG_COMMON \
	int started; \
	int status; \
	taplog_t taplog

#if defined(_WIN32) || defined(_WIN64)
#  include "tapcfg_windows.c"
#else
#  include "tapcfg_unix.c"
#endif

int
tapcfg_get_version()
{
	return TAPCFG_VERSION;
}

void
tapcfg_set_log_level(tapcfg_t *tapcfg, int level)
{
	taplog_set_level(&tapcfg->taplog, level);
}

void
tapcfg_set_log_callback(tapcfg_t *tapcfg, taplog_callback_t callback)
{
	taplog_set_callback(&tapcfg->taplog, callback);
}
