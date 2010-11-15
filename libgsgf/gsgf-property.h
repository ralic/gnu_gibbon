/*
 * This file is part of Gibbon, a graphical frontend to the First Internet 
 * Backgammon Server FIBS.
 * Copyright (C) 2009 Guido Flohr, http://guido-flohr.net/.
 *
 * Gibbon is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Gibbon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gibbon.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LIBGSGF_PROPERTY_H
# define _LIBGSGF_PROPERTY_H

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GSGF_TYPE_PROPERTY             (gsgf_property_get_type ())
#define GSGF_PROPERTY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSGF_TYPE_PROPERTY, GSGFProperty))
#define GSGF_PROPERTY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSGF_TYPE_PROPERTY, GSGFPropertyClass))
#define GSGF_IS_PROPERTY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSGF_TYPE_PROPERTY))
#define GSGF_IS_PROPERTY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSGF_TYPE_PROPERTY))
#define GSGF_PROPERTY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GSGF_TYPE_PROPERTY, GSGFPropertyClass))

/**
 * GSGFProperty:
 *
 * Class representing a property of games resp. game trees in a
 * Simple Game Format (SGF) file.
 **/
typedef struct _GSGFPropertyClass   GSGFPropertyClass;
typedef struct _GSGFProperty        GSGFProperty;
typedef struct _GSGFPropertyPrivate GSGFPropertyPrivate;

typedef enum gsgf_property_type {
        GSGF_PROPERTY_TEXT,
        GSGF_PROPERTY_NUMBER,
        GSGF_PROPERTY_REAL
} GSGFPropertyType;

struct _GSGFPropertyClass
{
        GObjectClass parent_class;
};

GType gsgf_property_get_type(void) G_GNUC_CONST;

struct _GSGFProperty
{
        GObject parent_instance;

        /*< private >*/
        GSGFPropertyPrivate *priv;
};

GSGFProperty *gsgf_property_new();

gboolean gsgf_property_add_value(GSGFProperty *property, const gchar *text, GError **error);
GList *gsgf_property_get_values(GSGFProperty *property);

G_END_DECLS

#endif
