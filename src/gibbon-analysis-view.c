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
#include "gibbon-roll.h"
#include "gibbon-move.h"
#include "gibbon-settings.h"
#include "gibbon-met.h"

typedef struct _GibbonAnalysisViewPrivate GibbonAnalysisViewPrivate;
struct _GibbonAnalysisViewPrivate {
        const GibbonApp *app;

        GtkBox *detail_box;
        GtkNotebook *notebook;
        GtkButtonBox *button_box;

        GtkLabel *move_summary;
        GtkLabel *cube_summary;

        GtkLabel *cube_equity_summary;

        GtkLabel *cube_win;
        GtkLabel *cube_win_g;
        GtkLabel *cube_win_bg;
        GtkLabel *cube_lose;
        GtkLabel *cube_lose_g;
        GtkLabel *cube_lose_bg;

        GtkToggleButton *show_equity;

        GibbonAnalysisMove *ma;
};

#define GIBBON_ANALYSIS_VIEW_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        GIBBON_TYPE_ANALYSIS_VIEW, GibbonAnalysisViewPrivate))

G_DEFINE_TYPE (GibbonAnalysisView, gibbon_analysis_view, G_TYPE_OBJECT)

static void gibbon_analysis_view_set_move (GibbonAnalysisView *self,
                                           GibbonAnalysisMove *a);
static void gibbon_analysis_view_set_move_mwc (GibbonAnalysisView *self);
static void gibbon_analysis_view_set_move_equity (GibbonAnalysisView *self);
static void gibbon_analysis_view_set_roll (GibbonAnalysisView *self,
                                           GibbonAnalysisRoll *a);
static void gibbon_analysis_view_on_toggle_show_mwc (GibbonAnalysisView *self);

static void 
gibbon_analysis_view_init (GibbonAnalysisView *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                GIBBON_TYPE_ANALYSIS_VIEW, GibbonAnalysisViewPrivate);

        self->priv->app = NULL;

        self->priv->detail_box = NULL;
        self->priv->notebook = NULL;
        self->priv->button_box = NULL;

        self->priv->move_summary = NULL;
        self->priv->cube_summary = NULL;

        self->priv->cube_equity_summary = NULL;

        self->priv->cube_win = NULL;
        self->priv->cube_win_g = NULL;
        self->priv->cube_win_bg = NULL;
        self->priv->cube_lose = NULL;
        self->priv->cube_lose_g = NULL;
        self->priv->cube_lose_bg = NULL;

        self->priv->ma = NULL;
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
        GSettings *settings;

        self->priv->app = app;

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

        obj = gibbon_app_find_object (app, "label-cube-equity-summary",
                                      GTK_TYPE_LABEL);
        self->priv->cube_equity_summary = GTK_LABEL (obj);

        obj = gibbon_app_find_object (app, "label-cube-win",
                                      GTK_TYPE_LABEL);
        self->priv->cube_win = GTK_LABEL (obj);
        obj = gibbon_app_find_object (app, "label-cube-win-g",
                                      GTK_TYPE_LABEL);
        self->priv->cube_win_g = GTK_LABEL (obj);
        obj = gibbon_app_find_object (app, "label-cube-win-bg",
                                      GTK_TYPE_LABEL);
        self->priv->cube_win_bg = GTK_LABEL (obj);
        obj = gibbon_app_find_object (app, "label-cube-lose",
                                      GTK_TYPE_LABEL);
        self->priv->cube_lose = GTK_LABEL (obj);
        obj = gibbon_app_find_object (app, "label-cube-lose-g",
                                      GTK_TYPE_LABEL);
        self->priv->cube_lose_g = GTK_LABEL (obj);
        obj = gibbon_app_find_object (app, "label-cube-lose-bg",
                                      GTK_TYPE_LABEL);
        self->priv->cube_lose_bg = GTK_LABEL (obj);

        obj = gibbon_app_find_object (app, "toggle-mwc-eq",
                                      GTK_TYPE_TOGGLE_BUTTON);
        self->priv->show_equity = GTK_TOGGLE_BUTTON (obj);
        settings = g_settings_new (GIBBON_PREFS_MATCH_SCHEMA);
        g_settings_bind (settings, GIBBON_PREFS_MATCH_SHOW_EQUITY, obj,
                         "active",
                         G_SETTINGS_BIND_DEFAULT
                         | G_SETTINGS_BIND_INVERT_BOOLEAN);
        g_signal_connect_swapped (obj, "toggled",
                                  (GCallback)
                                  gibbon_analysis_view_on_toggle_show_mwc,
                                  self);

        return self;
}

void
gibbon_analysis_view_set_analysis (GibbonAnalysisView *self,
                                   const GibbonGame *game, guint action_number)
{
        gint i;
        const GibbonGameAction *action;
        const GibbonAnalysis *move_analysis = NULL;
        const GibbonAnalysis *roll_analysis = NULL;

        g_return_if_fail (GIBBON_IS_ANALYSIS_VIEW (self));

        /* First find the corresponding roll.  */
        for (i = action_number; i >= 0; --i) {
                action = gibbon_game_get_nth_action (game, i, NULL);
                if (GIBBON_IS_ROLL (action)) {
                        roll_analysis = gibbon_game_get_nth_analysis (game, i);
                        break;
                }
        }

        /* Then find the corresponding move.  */
        for (i = action_number; ; ++i) {
                action = gibbon_game_get_nth_action (game, i, NULL);
                if (!action)
                        break;
                if (GIBBON_IS_MOVE (action)) {
                        move_analysis = gibbon_game_get_nth_analysis (game, i);
                        break;
                }
        }

        if (roll_analysis) {
                gibbon_analysis_view_set_roll (self, GIBBON_ANALYSIS_ROLL (
                                roll_analysis));
        } else {
                gtk_widget_hide (GTK_WIDGET (self->priv->detail_box));
        }

        if (move_analysis) {
                gibbon_analysis_view_set_move (self, GIBBON_ANALYSIS_MOVE (
                                move_analysis));
        } else {
                gtk_widget_hide (GTK_WIDGET (self->priv->notebook));
                gtk_widget_hide (GTK_WIDGET (self->priv->button_box));
        }

        action = gibbon_game_get_nth_action (game, action_number, NULL);
        if (action) {
                if (GIBBON_IS_ROLL (action) && roll_analysis
                    && move_analysis
                    && GIBBON_ANALYSIS_MOVE (move_analysis)->da) {
                        gtk_notebook_set_current_page (self->priv->notebook, 0);
                } else if (move_analysis) {
                        gtk_notebook_set_current_page (self->priv->notebook, 1);
                }
        }

        if (!move_analysis || !GIBBON_ANALYSIS_MOVE (move_analysis)->da) {
                /* FIXME! Disable this page! */
        }
}

static void
gibbon_analysis_view_set_move (GibbonAnalysisView *self, GibbonAnalysisMove *a)
{
        gchar *buf;
        gboolean show_mwc;

        if (self->priv->ma)
                g_object_unref (self->priv->ma);
        self->priv->ma = g_object_ref (a);

        gtk_widget_show (GTK_WIDGET (self->priv->detail_box));
        gtk_widget_show (GTK_WIDGET (self->priv->notebook));
        gtk_widget_show (GTK_WIDGET (self->priv->button_box));

        buf = g_strdup_printf ("%.2f %%",
                               100 * a->da_p[0][GIBBON_ANALYSIS_MOVE_DA_PWIN]);
        gtk_label_set_text (self->priv->cube_win, buf);
        g_free (buf);

        buf = g_strdup_printf ("%.2f %%",
                         100 * a->da_p[0][GIBBON_ANALYSIS_MOVE_DA_PWIN_GAMMON]);
        gtk_label_set_text (self->priv->cube_win_g, buf);
        g_free (buf);

        buf = g_strdup_printf ("%.2f %%",
                     100 * a->da_p[0][GIBBON_ANALYSIS_MOVE_DA_PWIN_BACKGAMMON]);
        gtk_label_set_text (self->priv->cube_win_bg, buf);
        g_free (buf);

        buf = g_strdup_printf ("%.2f %%", 100 * (1 - a->da_p[0][0]));
        gtk_label_set_text (self->priv->cube_lose, buf);
        g_free (buf);

        buf = g_strdup_printf ("%.2f %%", 100 * a->da_p[0][3]);
        gtk_label_set_text (self->priv->cube_lose_g, buf);
        g_free (buf);

        buf = g_strdup_printf ("%.2f %%", 100 * a->da_p[0][4]);
        gtk_label_set_text (self->priv->cube_lose_bg, buf);
        g_free (buf);

        g_object_get (G_OBJECT (self->priv->show_equity),
                      "active", &show_mwc,
                      NULL);

        if (show_mwc && a->match_length > 0)
                gibbon_analysis_view_set_move_mwc (self);
        else
                gibbon_analysis_view_set_move_equity (self);
}

static void
gibbon_analysis_view_set_move_mwc (GibbonAnalysisView *self)
{
        GibbonAnalysisMove *a = self->priv->ma;
        gchar *buf;
        gdouble equity = a->da_p[0][GIBBON_ANALYSIS_MOVE_DA_EQUITY];
        const GibbonMET *met;

        met = gibbon_app_get_met (self->priv->app);

        if (a->da_rollout) {
                buf = g_strdup_printf (_("Cubeless rollout MWC: %s (Money: %.3f)"),
                                       "???", equity);
        } else {
                buf = g_strdup_printf (
                        _("Cubeless %llu-ply MWC: %.2f %% (Money: %.3f)"),
                        (unsigned long long) a->da_plies,
                        100.0f * gibbon_met_get_mwc (met, equity,
                                                     a->match_length,
                                                     a->is_crawford,
                                                     a->my_score, a->opp_score),
                        equity);
        }
        gtk_label_set_text (self->priv->cube_equity_summary, buf);
        g_free (buf);
}

static void
gibbon_analysis_view_set_move_equity (GibbonAnalysisView *self)
{

}

void
gibbon_analysis_view_set_toggle_sensitive (GibbonAnalysisView *self,
                                           gboolean sensitive)
{
        g_return_if_fail (GIBBON_IS_ANALYSIS_VIEW (self));

        g_object_set (G_OBJECT (self->priv->show_equity),
                      "sensitive", sensitive,
                      NULL);
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

static void
gibbon_analysis_view_on_toggle_show_mwc (GibbonAnalysisView *self)
{
        g_printerr ("Toggled!\n");

        if (!self->priv->ma)
                return;

        /*
         * g_analysis_view_set_move() unreference an existing analysis.  We
         * temporarily increase the reference count, so that it cannot drop
         * to zero here.
         */
        g_object_ref (self->priv->ma);
        gibbon_analysis_view_set_move (self, self->priv->ma);
        g_object_unref (self->priv->ma);
}
