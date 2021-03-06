/* sna-utils.h
 * Definitions for SNA dissection.
 *
 * $Id: sna-utils.h 43536 2012-06-28 22:56:06Z darkjames $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __SNA_UTILS__
#define __SNA_UTILS__

#include <glib.h>
#include <stdio.h>

/*
 * Structure used to represent an FID Type 4 address; gives the layout of the
 * data pointed to by an AT_SNA "address" structure if the size is
 * SNA_FID_TYPE_4_ADDR_LEN.
 */
#define	SNA_FID_TYPE_4_ADDR_LEN	6
struct sna_fid_type_4_addr {
	guint32	saf;
	guint16	ef;
};

extern gchar *sna_fid_to_str(const address *addr);
extern void sna_fid_to_str_buf(const address *addr, gchar *buf, int buf_len);

#endif
