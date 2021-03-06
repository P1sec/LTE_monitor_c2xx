/* export_sslkeys.h
 *
 * $Id: export_sslkeys.h 45589 2012-10-16 18:14:16Z gerald $
 *
 * Export SSL Session Keys dialog
 * by Sake Blok <sake@euronet.nl> (20110526)
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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

#ifndef __EXPORT_SSLKEYS_H__
#define __EXPORT_SSLKEYS_H__

/** Callback for "Export SSL Session Keys" operation.
 *
 * @param w unused
 * @param data unused
 */
extern void savesslkeys_cb(GtkWidget * w, gpointer data);

#endif /* __EXPORT_SSLKEYS_H__ */
