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
 * SECTION:gibbon-analysis-view
 * @short_description: View components for analysis data!
 *
 * Since: 0.2.0
 *
 * The view components displaying move analysis data are shown and hidden
 * on demand, depending on whether analysis data is available for a particular
 * move or roll.
 */

#include <glib.h>
#include <glib/gi18n.h>

#include "gibbon-analysis-view.h"
#include "gibbon-analysis-roll.h"
#include "gibbon-analysis-move.h"

typedef struct _GibbonAnalysisViewPrivate GibbonAnalysisViewPrivate;
struct _GibbonAnalysisViewPrivate {
        GtkBox *detail_box;
        GtkNotebook *notebook;
        GtkButtonBox *button_box;

        GtkLabel *move_summary;
        GtkLabel *cube_summary;
};

#define GIBBON_ANALYSIS_VIEW_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        GIBBON_TYPE_ANALYSIS_VIEW, GibbonAnalysisViewPrivate))

G_DEFINE_TYPE (GibbonAnalysisView, gibbon_analysis_view, G_TYPE_OBJECT)

static void gibbon_analysis_view_set_move (GibbonAnalysisView *self,
                                           GibbonAnalysisMove *a);
static void gibbon_analysis_view_set_roll (GibbonAnalysisView *self,
                                           GibbonAnalysisRoll *a);

static void 
gibbon_analysis_view_init (GibbonAnalysisView *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                GIBBON_TYPE_ANALYSIS_VIEW, GibbonAnalysisViewPrivate);

        self->priv->detail_box = NULL;
        self->priv->notebook = NULL;
        self->priv->button_box = NULL;

        self->priv->move_summary = NULL;
        self->priv->cube_summary = NULL;
}

static void
gibbon_analysis_view_finalize (GObject *object)
{
        G_OBJECT_CLASS (gibbon_analysis_view_parent_class)->finalize(object);
}

static void
gibbon_analysis_view_class_init (GibbonAnalysisViewClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        
        g_type_class_add_private (klass, sizeof (GibbonAnalysisViewPrivate));

        object_class->finalize = gibbon_analysis_view_finalize;
}

/**
 * gibbon_analysis_view_new:
 * @app: The GibbonApp.
 *
 * Creates a new #GibbonAnalysisView.
 *
 * Returns: The newly created #GibbonAnalysisView or %NULL in case of failure.
 */
GibbonAnalysisView *
gibbon_analysis_view_new (const GibbonApp *app)
{
        GibbonAnalysisView *self = g_object_new (GIBBON_TYPE_ANALYSIS_VIEW,
                                                 NULL);
        GObject *obj;

        obj = gibbon_app_find_object (app, "hbox-analysis-detail",
                                      GTK_TYPE_BOX);
        gtk_widget_hide (GTK_WIDGET (obj));
        self->priv->detail_box = GTK_BOX (obj);

        obj = gibbon_app_find_object (app, "label-move-summary",
                                      GTK_TYPE_LABEL);
        self->priv->move_summary = GTK_LABEL (obj);
        obj = gibbon_app_find_object (app, "label-cube-summary",
                                      GTK_TYPE_LABEL);
        self->priv->cube_summary = GTK_LABEL (obj);

        obj = gibbon_app_find_object (app, "notebook-analysis",
                                      GTK_TYPE_NOTEBOOK);
        gtk_widget_hide (GTK_WIDGET (obj));
        self->priv->notebook = GTK_NOTEBOOK (obj);

        obj = gibbon_app_find_object (app, "hbuttonbox-analysis",
                                      GTK_TYPE_BUTTON_BOX);
        gtk_widget_hide (GTK_WIDGET (obj));
        self->priv->button_box = GTK_BUTTON_BOX (obj);

        return self;
}

void
gibbon_analysis_view_set_analysis (GibbonAnalysisView *self,
                                   const GibbonAnalysis* a)
{
        g_return_if_fail (GIBBON_IS_ANALYSIS_VIEW (self));
        if (a)
                g_return_if_fail (GIBBON_IS_ANALYSIS (a));

        if (!a) {
                gtk_widget_hide (GTK_WIDGET (self->priv->detail_box));
                gtk_widget_hide (GTK_WIDGET (self->priv->notebook));
                gtk_widget_hide (GTK_WIDGET (self->priv->button_box));
                return;
        } else if (GIBBON_IS_ANALYSIS_MOVE (a)) {
                gibbon_analysis_view_set_move (self, GIBBON_ANALYSIS_MOVE (a));
        } else if (GIBBON_IS_ANALYSIS_ROLL (a)) {
                gibbon_analysis_view_set_roll (self, GIBBON_ANALYSIS_ROLL (a));
        }
}

static void
gibbon_analysis_view_set_move (GibbonAnalysisView *self, GibbonAnalysisMove *a)
{
        gtk_widget_show (GTK_WIDGET (self->priv->detail_box));
        gtk_widget_show (GTK_WIDGET (self->priv->notebook));
        gtk_widget_show (GTK_WIDGET (self->priv->button_box));
}

static void
gibbon_analysis_view_set_roll (GibbonAnalysisView *self, GibbonAnalysisRoll *a)
{
        GibbonAnalysisRollLuck luck_type;
        gchar *text;
        gdouble luck_value;

        gtk_widget_show (GTK_WIDGET (self->priv->detail_box));
        gtk_widget_hide (GTK_WIDGET (self->priv->notebook));
        gtk_widget_hide (GTK_WIDGET (self->priv->button_box));

        gtk_label_set_text (self->priv->move_summary, NULL);

        luck_type = gibbon_analysis_roll_get_luck_type (a);
        luck_value = gibbon_analysis_roll_get_luck_value (a);
        switch (luck_type) {
        default:
                text = g_strdup_printf (_("Luck: %f"), luck_value);
                break;
        case GIBBON_ANALYSIS_ROLL_LUCK_LUCKY:
                text = g_strdup_printf (_("Luck: %f (lucky)"), luck_value);
                break;
        case GIBBON_ANALYSIS_ROLL_LUCK_VERY_LUCKY:
                text = g_strdup_printf (_("Luck: %f (very lucky)"), luck_value);
                break;
        case GIBBON_ANALYSIS_ROLL_LUCK_UNLUCKY:
                text = g_strdup_printf (_("Luck: %f (unlucky)"), luck_value);
                break;
        case GIBBON_ANALYSIS_ROLL_LUCK_VERY_UNLUCKY:
                text = g_strdup_printf (_("Luck: %f (very unlucky)"),
                                        luck_value);
                break;
        }

        gtk_label_set_text (self->priv->cube_summary, text);
        g_free (text);
}