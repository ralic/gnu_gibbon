/*
 * This file is part of gibbon.
 * Gibbon is a Gtk+ frontend for the First Internet Backgammon Server FIBS.
 * Copyright (C) 2009-2011 Guido Flohr, http://guido-flohr.net/.
 *
 * gibbon is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gibbon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gibbon.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GIBBON_UTIL_H
# define _GIBBON_UTIL_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

G_BEGIN_DECLS

enum GibbonClientType {
        GibbonClientUnknown = 0,
        GibbonClientGibbon = 1,
        GibbonClientRegular = 2,
        GibbonClientBot = 3,
        GibbonClientDaemon = 4,
        GibbonClientMobile = 5
};

gchar **gibbon_strsplit_ws (const gchar *string);
const gchar *gibbon_skip_ws_tokens (const gchar *string,
                                    const gchar * const * const tokens,
                                    gsize num);
enum GibbonClientType gibbon_get_client_type (const gchar *client_name,
                                              const gchar *user_name,
                                              const gchar *host_name,
                                              guint port);
void gibbon_safe_object_unref (gpointer data);

G_END_DECLS

#endif
