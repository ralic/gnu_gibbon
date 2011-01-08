/*
 * This file is part of Gibbon, a graphical frontend to the First Internet 
 * Backgammon Server FIBS.
 * Copyright (C) 2009-2010 Guido Flohr, http://guido-flohr.net/.
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

/**
 * SECTION:gsgf-point
 * @short_description: Abstract base class for an SGF point.
 *
 * The <link linkend="http://www.red-bean.com/sgf/sgf4.html">http://www.red-bean.com/sgf/sgf4.html</link>
 * specification does not define the details for a point.  The specific
 * point incarnations should inherit from this class.
 */

#include <glib.h>
#include <glib/gi18n.h>

#include <libgsgf/gsgf.h>

G_DEFINE_TYPE (GSGFPoint, gsgf_point, GSGF_TYPE_COOKED_VALUE)

static void
gsgf_point_init(GSGFPoint *self)
{
}

static void
gsgf_point_finalize(GObject *object)
{
        G_OBJECT_CLASS (gsgf_point_parent_class)->finalize(object);
}

static void
gsgf_point_class_init(GSGFPointClass *klass)
{
        GObjectClass* object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gsgf_point_finalize;
}
