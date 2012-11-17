/*
 * This file is part of gibbon.
 * Gibbon is a Gtk+ frontend for the First Internet Backgammon Server FIBS.
 * Copyright (C) 2009-2012 Guido Flohr, http://guido-flohr.net/.
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

/**
 * SECTION:gibbon-java-fibs-importer
 * @short_description: Import settings and games from JavaFIBS!
 *
 * Since: 0.1.0
 *
 * Class that handles data import from JavaFIBS.
 **/

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gibbon-app.h"
#include "gibbon-java-fibs-importer.h"
#include "gibbon-archive.h"
#include "gibbon-util.h"

typedef struct _GibbonJavaFIBSImporterPrivate GibbonJavaFIBSImporterPrivate;
struct _GibbonJavaFIBSImporterPrivate {
        gboolean running;

        GibbonArchive *archive;
        gchar *directory;
        gchar *user;
        gchar *server;
        guint port;
        gchar *password;

        GtkWidget *progress;
        gchar *status;

        guint okay_handler;
        guint stop_handler;

        GThread *worker;
        GMutex mutex;
        gint jobs;
        gint finished;
        guint timeout;
        gboolean cancelled;
};

#define GIBBON_JAVA_FIBS_IMPORTER_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        GIBBON_TYPE_JAVA_FIBS_IMPORTER, GibbonJavaFIBSImporterPrivate))

G_DEFINE_TYPE (GibbonJavaFIBSImporter, gibbon_java_fibs_importer, G_TYPE_OBJECT)

static gboolean gibbon_java_fibs_importer_select_directory (
                GibbonJavaFIBSImporter *self);
static gboolean gibbon_java_fibs_importer_check_directory (
                GibbonJavaFIBSImporter *self);
static gboolean gibbon_java_fibs_importer_select_user (
                GibbonJavaFIBSImporter *self);
static gboolean gibbon_java_fibs_importer_read_prefs (GibbonJavaFIBSImporter
                                                      *self,
                                                      gchar *path_to_prefs,
                                                      gchar **server,
                                                      guint *port,
                                                      gchar **password,
                                                      GError **error);
static gchar *gibbon_java_fibs_importer_decrypt (GibbonJavaFIBSImporter *self,
                                                 guint32 *buffer,
                                                 gsize password_length);
static void gibbon_java_fibs_importer_on_stop (GibbonJavaFIBSImporter *self);
static void gibbon_java_fibs_importer_on_okay (GibbonJavaFIBSImporter *self);
static gpointer gibbon_java_fibs_importer_work (GibbonJavaFIBSImporter *self);
static gboolean gibbon_java_fibs_importer_poll (GibbonJavaFIBSImporter *self);
static void gibbon_java_fibs_importer_ready (GibbonJavaFIBSImporter *self);

static void 
gibbon_java_fibs_importer_init (GibbonJavaFIBSImporter *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                GIBBON_TYPE_JAVA_FIBS_IMPORTER, GibbonJavaFIBSImporterPrivate);

        self->priv->running = FALSE;

        self->priv->archive = NULL;
        self->priv->directory = NULL;
        self->priv->user = NULL;
        self->priv->server = NULL;
        self->priv->port = 0;
        self->priv->password = NULL;

        self->priv->progress = NULL;
        self->priv->status = NULL;

        self->priv->worker = NULL;
        self->priv->jobs = -1;
        self->priv->finished = 0;
        self->priv->timeout = 0;
        self->priv->cancelled = FALSE;

        self->priv->okay_handler = 0;
        self->priv->stop_handler = 0;
}

static void
gibbon_java_fibs_importer_finalize (GObject *object)
{
        GibbonJavaFIBSImporter *self = GIBBON_JAVA_FIBS_IMPORTER (object);
        GObject *obj;

        if (self->priv->stop_handler) {
                obj = gibbon_app_find_object (app, "java-fibs-importer-stop",
                                              GTK_TYPE_BUTTON);
                g_signal_handler_disconnect (obj, self->priv->stop_handler);
        }

        if (self->priv->okay_handler) {
                obj = gibbon_app_find_object (app, "java-fibs-importer-ok",
                                              GTK_TYPE_BUTTON);
                g_signal_handler_disconnect (obj, self->priv->okay_handler);
        }

        if (self->priv->timeout)
                g_source_remove (self->priv->timeout);

        if (self->priv->worker)
                g_thread_join (self->priv->worker);

        if (self->priv->archive)
                g_object_unref (self->priv->archive);
        obj = gibbon_app_find_object (app, "import-java-fibs-menu-item",
                                      GTK_TYPE_MENU_ITEM);
        gtk_widget_set_sensitive (GTK_WIDGET (obj), TRUE);

        g_free (self->priv->status);

        g_free (self->priv->directory);
        g_free (self->priv->user);
        g_free (self->priv->server);
        g_free (self->priv->password);

        G_OBJECT_CLASS (gibbon_java_fibs_importer_parent_class)->finalize(object);
}

static void
gibbon_java_fibs_importer_class_init (GibbonJavaFIBSImporterClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        
        g_type_class_add_private (klass, sizeof (GibbonJavaFIBSImporterPrivate));

        object_class->finalize = gibbon_java_fibs_importer_finalize;
}

GibbonJavaFIBSImporter *
gibbon_java_fibs_importer_new ()
{
        GibbonJavaFIBSImporter *self =
                        g_object_new (GIBBON_TYPE_JAVA_FIBS_IMPORTER, NULL);

        self->priv->progress = gibbon_app_find_widget (
                        app, "java-fibs-importer-progressbar",
                        GTK_TYPE_PROGRESS_BAR);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (self->priv->progress),
                                       0.0);
        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->priv->progress));
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (self->priv->progress),
                                   NULL);

        return self;
}

void
gibbon_java_fibs_importer_run (GibbonJavaFIBSImporter *self)
{
        GtkWidget *dialog;
        gint reply;
        GtkWidget *widget;
        GError *error = NULL;
        guint id;

        g_return_if_fail (GIBBON_IS_JAVA_FIBS_IMPORTER (self));

        if (self->priv->running)
                return;

        self->priv->running = TRUE;

        dialog = gtk_message_dialog_new_with_markup (
                                GTK_WINDOW (gibbon_app_get_window (app)),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_QUESTION,
                                GTK_BUTTONS_OK_CANCEL,
                                "<span weight='bold' size='larger'>"
                                "%s</span>\n%s",
                                _("Import from JavaFIBS"),
                                _("In order to import your saved matches and"
                                  " settings from JavaFIBS you first have to"
                                  " select the JavaFIBS installation"
                                  " directory (that is the directory that"
                                  " contains the .jar file)."));

        reply = gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(GTK_WIDGET (dialog));

        if (reply != GTK_RESPONSE_OK) {
                g_object_unref (self);
                return;
        }

        if (!gibbon_java_fibs_importer_select_directory (self)) {
                g_object_unref (self);
                return;
        }

        dialog = gibbon_app_find_widget (app, "java-fibs-importer-dialog",
                                         GTK_TYPE_DIALOG);

        widget = gibbon_app_find_widget (app, "java-fibs-importer-stop",
                                         GTK_TYPE_BUTTON);
        self->priv->stop_handler = g_signal_connect_swapped (
                                G_OBJECT (widget), "clicked",
                                G_CALLBACK (gibbon_java_fibs_importer_on_stop),
                                self);
        gtk_widget_set_sensitive (widget, TRUE);

        widget = gibbon_app_find_widget (app, "java-fibs-importer-ok",
                                         GTK_TYPE_BUTTON);
        self->priv->okay_handler =
                        g_signal_connect_swapped (
                                        G_OBJECT (widget), "clicked",
                                        G_CALLBACK (gibbon_java_fibs_importer_on_okay),
                                        self);
        gtk_widget_set_sensitive (widget, FALSE);

        g_mutex_init (&self->priv->mutex);
        id = g_timeout_add (10, (GSourceFunc) gibbon_java_fibs_importer_poll,
                            self);
        self->priv->timeout = id;

        gtk_widget_show_all (dialog);

        if (!g_thread_try_new ("java-fibs-importer-worker",
                               (GThreadFunc) gibbon_java_fibs_importer_work,
                               self, &error)) {
                gibbon_app_display_error (app, _("System Error"),
                                          _("Error creating new thread: %s!"),
                                            error->message);
                g_object_unref (self);
                gtk_widget_hide (dialog);

                return;
        }
}

static gboolean
gibbon_java_fibs_importer_select_directory (GibbonJavaFIBSImporter *self)
{
        GtkWidget *dialog;
        gint reply;
        gchar *path;

        dialog = gtk_file_chooser_dialog_new (
                        _("Select JavaFIBS Directory"),
                        GTK_WINDOW (gibbon_app_get_window (app)),
                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                        NULL);
        gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER (dialog), FALSE);
        gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), FALSE);
        g_printerr ("Remove line %s:%d!\n", __FILE__, __LINE__ + 1);
        gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                              "/home/guido/java/JavaFIBS2001",
                                              NULL);
        reply = gtk_dialog_run(GTK_DIALOG (dialog));
        if (reply != GTK_RESPONSE_OK) {
                gtk_widget_destroy (dialog);
                return FALSE;
        }

        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        gtk_widget_destroy (dialog);

        self->priv->directory = path;

        return gibbon_java_fibs_importer_check_directory (self);
}

static gboolean
gibbon_java_fibs_importer_check_directory (GibbonJavaFIBSImporter *self)
{
        gchar *path_to_last_user;
        GError *error = NULL;
        gchar *buffer;
        gsize bytes_read;

        /*
         * If we cannot find the file user/last.user
         */
        path_to_last_user = g_build_filename (self->priv->directory,
                                              "user", "last.user", NULL);
        if (!g_file_test (path_to_last_user, G_FILE_TEST_EXISTS)) {
                gibbon_app_display_error (app, _("Not a JavaFIBS Installation"),
                                          _("The directory `%s' does not look"
                                            " like a JavaFIBS installation. "
                                            " The file `%s' is missing."),
                                            self->priv->directory,
                                            path_to_last_user);
                g_free (path_to_last_user);
                return gibbon_java_fibs_importer_select_directory (self);
        }

        if (!gibbon_slurp_file (path_to_last_user, &buffer, &bytes_read,
                                NULL, &error)) {
                gibbon_app_display_error (app, _("Read Error"),
                                          _("Error reading `%s': %s!"),
                                            path_to_last_user,
                                            error->message);
                g_free (path_to_last_user);
                g_error_free (error);
                /* No point falling back to last step here.  */;
                return FALSE;
        }

        g_free (path_to_last_user);

        buffer = g_realloc (buffer, bytes_read + 1);
        buffer[bytes_read] = 0;

        self->priv->user = g_strdup (gibbon_trim (buffer));
        if (!*self->priv->user) {
                g_free (self->priv->user);
                self->priv->user = NULL;
        }
        g_free (buffer);

        return gibbon_java_fibs_importer_select_user (self);
}

static gboolean
gibbon_java_fibs_importer_select_user (GibbonJavaFIBSImporter *self)
{
        gchar *path_to_user = g_build_filename (self->priv->directory,
                                                "user", NULL);
        GError *error = NULL;
        GDir *dir = g_dir_open (path_to_user, 0, &error);
        const gchar *user_dir;
        gchar *path_to_prefs;
        gchar *server; guint port; gchar *password;
        GtkListStore *store;
        GtkTreeIter iter;
        gsize n_users = 0;
        gint active = -1;
        gchar *text;
        GtkWidget *window, *dialog, *content_area, *label, *combo;
        GtkCellRenderer *cell;
        gint response;

        if (!dir) {
                gibbon_app_display_error (app, NULL,
                                          _("Error opening directory `%s': %s!"),
                                            path_to_user,
                                            error->message);
                g_free (path_to_user);
                g_error_free (error);
                /* No point falling back to last step here.  */;
                return FALSE;
        }
        store = gtk_list_store_new (5,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_UINT,
                                    G_TYPE_STRING);

        do {
                user_dir = g_dir_read_name (dir);
                if (!user_dir)
                        break;
                if (user_dir[0] == '.')
                        continue;
                path_to_prefs = g_build_filename (path_to_user, user_dir,
                                                  "preferences", NULL);
                if (!g_file_test (path_to_prefs, G_FILE_TEST_EXISTS))
                        continue;

                if (!gibbon_java_fibs_importer_read_prefs (self, path_to_prefs,
                                                           &server, &port,
                                                           &password, NULL))
                        continue;
                gtk_list_store_append (store, &iter);
                text = g_strdup_printf (_("%s on %s port %u"),
                                        user_dir, server, port);
                gtk_list_store_set (store, &iter,
                                    0, text,
                                    1, user_dir,
                                    2, server,
                                    3, port,
                                    4, password,
                                    -1);
                g_free (text);

                if (!g_strcmp0 (user_dir, self->priv->user))
                        active = n_users;

                ++n_users;
        } while (user_dir != NULL);

        g_dir_close (dir);

        if (!n_users) {
                gibbon_app_display_error (app, NULL,
                                          _("No user data found in `%s'!"),
                                            path_to_user);
                g_object_unref (store);
                g_free (path_to_user);
                return FALSE;
        } else if (n_users == 1) {
                /* Iter was initialized above.  */
                g_free (self->priv->user);
                gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                                    1, &self->priv->user,
                                    2, &self->priv->server,
                                    3, &self->priv->port,
                                    4, &self->priv->password,
                                    -1);
                g_object_unref (store);
                g_free (path_to_user);

                return TRUE;
        }

        window = gibbon_app_get_window (app);
        dialog = gtk_dialog_new_with_buttons (_("Select Account"),
                                              GTK_WINDOW (window),
                                              GTK_DIALOG_DESTROY_WITH_PARENT
                                              | GTK_DIALOG_MODAL,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_OK,
                                              NULL);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        label = gtk_label_new (_("Multiple accounts found.   Please select"
                                 " one identity:"));
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_container_add (GTK_CONTAINER (content_area), label);

        combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
        cell = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                        "text", 0,
                                        NULL);

        gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (combo), 0);
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), active);

        gtk_container_add (GTK_CONTAINER (content_area), combo);

        gtk_widget_show_all (dialog);

        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

        if (response != GTK_RESPONSE_OK
            || !gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter)) {
                gtk_widget_destroy (dialog);
                g_object_unref (store);
                g_free (path_to_user);
                return FALSE;
        }

        gtk_widget_destroy (dialog);

        gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                            1, &self->priv->user,
                            2, &self->priv->server,
                            3, &self->priv->port,
                            4, &self->priv->password,
                            -1);

        g_object_unref (store);
        g_free (path_to_user);

        return TRUE;
}

static gboolean
gibbon_java_fibs_importer_read_prefs (GibbonJavaFIBSImporter *self,
                                      gchar *path_to_prefs,
                                      gchar **server,
                                      guint *port,
                                      gchar **password,
                                      GError **error)
{
        gsize filesize;
        gchar *buffer;
        guint32 *ui32;
        guint32 password_length, server_length;
        gchar *server_utf16;
        gsize bytes_read, bytes_written;

        if (!gibbon_slurp_file (path_to_prefs, &buffer, &filesize,
                                NULL, error))
                return FALSE;

        if (filesize < 8) {
                g_set_error_literal (error, 0, -1, _("premature end of file"));
                g_free (buffer);
                return FALSE;
        }

        ui32 = (guint32 *) buffer;
        ++ui32;
        password_length = GUINT32_FROM_BE (*ui32);
        ++ui32;

        if (filesize < 8 + 4 * password_length) {
                g_set_error_literal (error, 0, -1, _("premature end of file"));
                g_free (buffer);
                return FALSE;
        }
        if (password_length) {
                *password = gibbon_java_fibs_importer_decrypt (self,
                                                               ui32,
                                                               password_length);
                ui32 += password_length;
        } else {
                *password = NULL;
        }

        server_length = GUINT32_FROM_BE (*ui32);
        ++ui32;

        if (filesize < 16 + 4 * password_length + 2 * server_length) {
                g_set_error_literal (error, 0, -1, _("premature end of file"));
                g_free (buffer);
                return FALSE;
        }
        if (!server_length) {
                g_set_error_literal (error, 0, -1, _("no server name"));
                g_free (buffer);
                return FALSE;
        }

        server_utf16 = (gchar *) ui32;
        *server = g_convert (server_utf16, 2 * server_length,
                             "utf-8", "utf-16be",
                             &bytes_read, &bytes_written, error);
        if (!*server) {
                g_free (buffer);
                return FALSE;
        }

        if (bytes_read != 2 * server_length
            || bytes_written != server_length) {
                g_set_error_literal (error, 0, 1, _("error converting hostname"));
                return FALSE;
        }
        server_utf16 += 2 * server_length;
        ui32 = (guint32 *) server_utf16;

        *port = GUINT32_FROM_BE (*ui32);

        g_free (buffer);

        return TRUE;
}

static gchar *
gibbon_java_fibs_importer_decrypt (GibbonJavaFIBSImporter *self,
                                   guint32 *buffer,
                                   gsize password_length)
{
        gchar *password = g_malloc (password_length + 1);
        guchar *dest;
        guint32 s;
        guint32 *src;
        gint i;

        /*
         * This routine only "decrypts" passwords with characters in the
         * range from 32 to 127.  Other characters are not allowed anyway.
         *
         * BTW, this is not really encryption.  JavaFIBS scrambles passwords
         * by converting characters to floats.  The restriction to codes
         * 32-127 makes parsing the floats very simple.
         */
        src = buffer;
        dest = (guchar *) password;
        for (i = 0; i < password_length; ++i) {
                s = *src++;
                s = GUINT32_FROM_BE (s);
                if ((s & 0xff000000) != 0x42000000) {
                        g_free (password);
                        return NULL;
                }
                s = s & 0xff0000;
                s >>= 16;

                if (s < 0x80) {
                        if (s & 0x3) {
                                g_free (password);
                                return NULL;
                        }
                        *dest++ = (s / 4) + 32;
                } else {
                        if (s & 0x1) {
                                g_free (password);
                                return NULL;
                        }
                        *dest++ = (s / 2);
                }
        }
        *dest = 0;

        return password;
}

static void
gibbon_java_fibs_importer_on_stop (GibbonJavaFIBSImporter *self)
{
        gibbon_java_fibs_importer_ready (self);

        g_mutex_lock (&self->priv->mutex);
        self->priv->cancelled = TRUE;
        g_mutex_unlock (&self->priv->mutex);
}

static void
gibbon_java_fibs_importer_on_okay (GibbonJavaFIBSImporter *self)
{
        GtkWidget *dialog;

        dialog = gibbon_app_find_widget (app, "java-fibs-importer-dialog",
                                         GTK_TYPE_DIALOG);
        gtk_widget_hide (dialog);

        g_object_unref (self);
}

static gpointer
gibbon_java_fibs_importer_work (GibbonJavaFIBSImporter *self)
{
        gint i;

        g_mutex_lock (&self->priv->mutex);
        self->priv->status = g_strdup (_("Collecting data"));
        g_mutex_unlock (&self->priv->mutex);

        g_usleep (G_USEC_PER_SEC);

        g_mutex_lock (&self->priv->mutex);
        self->priv->jobs = 10;
        self->priv->finished = 0;
        g_mutex_unlock (&self->priv->mutex);

        g_usleep (G_USEC_PER_SEC);

        for (i = 0; i < 10; ++i) {
                g_mutex_lock (&self->priv->mutex);
                if (self->priv->cancelled) {
                        g_mutex_unlock (&self->priv->mutex);
                        return NULL;
                }
                if (self->priv->status)
                        g_free (self->priv->status);
                self->priv->status = g_strdup_printf ("Doing thing #%u.",
                                                      self->priv->finished + 1);
                ++self->priv->finished;
                g_mutex_unlock (&self->priv->mutex);

                g_usleep (G_USEC_PER_SEC);
        }

        return NULL;
}

static gboolean
gibbon_java_fibs_importer_poll (GibbonJavaFIBSImporter *self)
{
        gdouble fraction;

        g_mutex_lock (&self->priv->mutex);

        if (self->priv->jobs <= 0)
                gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->priv->progress));
        else {
                fraction = (gdouble) self->priv->finished / self->priv->jobs;
                gtk_progress_bar_set_fraction (
                                GTK_PROGRESS_BAR (self->priv->progress),
                                fraction);
        }

        if (self->priv->status) {
                gtk_progress_bar_set_text (
                                GTK_PROGRESS_BAR (self->priv->progress),
                                self->priv->status);
                g_free (self->priv->status);
                self->priv->status = NULL;
        }

        if (self->priv->jobs == self->priv->finished) {
                gibbon_java_fibs_importer_ready (self);
                g_mutex_unlock (&self->priv->mutex);

                return FALSE;
        }

        g_mutex_unlock (&self->priv->mutex);

        return TRUE;
}

static void
gibbon_java_fibs_importer_ready (GibbonJavaFIBSImporter *self)
{
        GtkWidget *button;

        button = gibbon_app_find_widget (app, "java-fibs-importer-stop",
                                         GTK_TYPE_BUTTON);
        gtk_widget_set_sensitive (button, FALSE);
        button = gibbon_app_find_widget (app, "java-fibs-importer-ok",
                                         GTK_TYPE_BUTTON);
        gtk_widget_set_sensitive (button, TRUE);
}
