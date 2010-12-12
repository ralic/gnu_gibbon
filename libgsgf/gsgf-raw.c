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
 * SECTION:gsgf-raw
 * @short_description: Raw, unqualified data in SGF files.
 *
 * A #GSGFRaw is the only #GSGFCookedValue that is not a cooked value;
 * the name sort of suggests that.  You should never use a #GSGFRaw
 * directly.  It is internally used, when parsing data before it
 * gets cooked.
 */

#include <glib.h>
#include <glib/gi18n.h>

#include <libgsgf/gsgf.h>

struct _GSGFRawPrivate {
        GList *values;
};

#define GSGF_RAW_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      GSGF_TYPE_RAW,           \
                                      GSGFRawPrivate))

G_DEFINE_TYPE(GSGFRaw, gsgf_raw, GSGF_TYPE_COOKED_VALUE)

static gboolean gsgf_raw_write_stream(const GSGFCookedValue *self,
                                      GOutputStream *out, gsize *bytes_written,
                                      GCancellable *cancellable, GError **error);

static void
gsgf_raw_init(GSGFRaw *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                        GSGF_TYPE_RAW,
                        GSGFRawPrivate);

        self->priv->values = NULL;
}

static void
gsgf_raw_finalize(GObject *object)
{
        GSGFRaw *self = GSGF_RAW(object);

        if (self->priv->values) {
                g_list_foreach(self->priv->values, g_free, NULL);
                g_list_free(self->priv->values);
        }

        G_OBJECT_CLASS (gsgf_raw_parent_class)->finalize(object);
}

static void
gsgf_raw_class_init(GSGFRawClass *klass)
{
        GObjectClass* object_class = G_OBJECT_CLASS(klass);
        GSGFCookedValueClass *gsgf_cooked_value_class = GSGF_COOKED_VALUE_CLASS(klass);

        g_type_class_add_private(klass, sizeof(GSGFRawPrivate));

        gsgf_cooked_value_class->write_stream = gsgf_raw_write_stream;

        object_class->finalize = gsgf_raw_finalize;
}

/**
 * gsgf_raw_new:
 * @value: The value to store.
 *
 * Creates a new #GSGFRaw and stores a copy of @value.
 *
 * Returns: The new #GSGFRaw.
 */
GSGFRaw *
gsgf_raw_new (const gchar *value)
{
        GSGFRaw *self = g_object_new(GSGF_TYPE_RAW, NULL);

        if (value)
                self->priv->values = g_list_append(self->priv->values, g_strdup(value));

        return self;
}

gchar *
_gsgf_raw_get_value(const GSGFRaw *self, gsize i)
{
        return (gchar *) g_list_nth_data(self->priv->values, i);
}

/* FIXME! Raw values must not implement the serialization method! */
static gboolean
gsgf_raw_write_stream(const GSGFCookedValue *_self,
                      GOutputStream *out, gsize *bytes_written,
                      GCancellable *cancellable, GError **error)
{
        gsize written_here;
        GList *iter;
        gchar *value;
        GSGFRaw *self = GSGF_RAW(_self);

        *bytes_written = 0;

        iter = self->priv->values;

        if (!iter) {
                g_set_error(error, GSGF_ERROR, GSGF_ERROR_EMPTY_PROPERTY,
                            _("Attempt to write empty property"));
                return FALSE;
        }

        while (iter) {
                if (!g_output_stream_write_all(out, "[", 1, &written_here,
                                               cancellable, error)) {
                        *bytes_written += written_here;
                        return FALSE;
                }
                *bytes_written += written_here;

                value = (gchar *) iter->data;
                if (!g_output_stream_write_all(out, value, strlen(value),
                                               bytes_written,
                                               cancellable, error)) {
                        *bytes_written += written_here;
                        return FALSE;
                }
                *bytes_written += written_here;

                if (!g_output_stream_write_all(out, "]", 1, &written_here,
                                               cancellable, error)) {
                        *bytes_written += written_here;
                        return FALSE;
                }
                *bytes_written += written_here;

                iter = iter->next;
        }

        return TRUE;
}

gboolean
_gsgf_raw_convert(GSGFRaw *self, const gchar *charset, GError **error)
{
        gchar *converted;
        gsize bytes_written;
        GList *iter;
        gchar *value;

        if (error)
                *error = NULL;

        iter = self->priv->values;

        while (iter) {
                value = (gchar *) iter->data;

                converted = g_convert(value, -1, "UTF-8", charset,
                                      NULL, &bytes_written, NULL);
                if (!converted)
                        return FALSE;

                iter->data = (gpointer) converted;

                iter = iter->next;
        }

        return TRUE;
}

void _gsgf_raw_add_value(const GSGFCookedValue *_self, const gchar *value)
{
        GSGFRaw *self = GSGF_RAW(_self);

        self->priv->values = g_list_append(self->priv->values, g_strdup(value));
}
