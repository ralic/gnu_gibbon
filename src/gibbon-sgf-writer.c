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
 * SECTION:gibbon-sgf-writer
 * @short_description: Convert a #GibbonMatch to SGF.
 *
 * Since: 0.1.1
 *
 * A #GibbonMatchWriter for SGF.
 */

#include <glib.h>
#include <glib/gi18n.h>

#include <libgsgf/gsgf.h>

#include "gibbon-sgf-writer.h"
#include "gibbon-match.h"
#include "gibbon-game.h"

#include "gibbon-roll.h"
#include "gibbon-move.h"
#include "gibbon-double.h"
#include "gibbon-drop.h"
#include "gibbon-take.h"

G_DEFINE_TYPE (GibbonSGFWriter, gibbon_sgf_writer, GIBBON_TYPE_MATCH_WRITER)

static gboolean gibbon_sgf_writer_write_stream (const GibbonMatchWriter *writer,
                                                GOutputStream *out,
                                                const GibbonMatch *match,
                                                GError **error);
static gboolean gibbon_sgf_writer_write_game (const GibbonSGFWriter *self,
                                              GSGFGameTree *game_tree,
                                              const GibbonGame *game,
                                              guint game_number,
                                              const GibbonMatch *match,
                                              GError **error);
static gboolean gibbon_sgf_writer_roll (const GibbonSGFWriter *self,
                                        GSGFGameTree *game_tree,
                                        GibbonPositionSide side,
                                        GibbonRoll *roll, GError **error);
static gboolean gibbon_sgf_writer_move (const GibbonSGFWriter *self,
                                        GSGFGameTree *game_tree,
                                        GibbonPositionSide side,
                                        GibbonMove *move, GError **error);
static gboolean gibbon_sgf_writer_special_move (const GibbonSGFWriter *self,
                                                GSGFGameTree *game_tree,
                                                GibbonPositionSide side,
                                                const gchar *special,
                                                GError **error);
static guint translate_point (guint point, GibbonPositionSide side);

static void 
gibbon_sgf_writer_init (GibbonSGFWriter *self)
{
}

static void
gibbon_sgf_writer_finalize (GObject *object)
{
        G_OBJECT_CLASS (gibbon_sgf_writer_parent_class)->finalize(object);
}

static void
gibbon_sgf_writer_class_init (GibbonSGFWriterClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GibbonMatchWriterClass *gibbon_match_writer_class =
                        GIBBON_MATCH_WRITER_CLASS (klass);

        gibbon_match_writer_class->write_stream =
                        gibbon_sgf_writer_write_stream;
        
        object_class->finalize = gibbon_sgf_writer_finalize;
}

/**
 * gibbon_sgf_writer_new:
 *
 * Creates a new #GibbonSGFWriter.
 *
 * Returns: The newly created #GibbonSGFWriter or %NULL in case of failure.
 */
GibbonSGFWriter *
gibbon_sgf_writer_new (void)
{
        GibbonSGFWriter *self = g_object_new (GIBBON_TYPE_SGF_WRITER, NULL);

        return self;
}

static gboolean
gibbon_sgf_writer_write_stream (const GibbonMatchWriter *_self,
                                GOutputStream *out, const GibbonMatch *match,
                                GError **error)
{
        const GibbonSGFWriter *self;
        GibbonGame *game;
        GSGFFlavor *flavor;
        GSGFCollection *collection;
        GSGFGameTree *game_tree;
        gsize bytes_written;
        gsize game_number;

        self = GIBBON_SGF_WRITER (_self);
        g_return_val_if_fail (self != NULL, FALSE);

        game = gibbon_match_get_current_game (match);
        if (!game) {
                g_set_error_literal (error, GIBBON_MATCH_ERROR,
                                     GIBBON_MATCH_ERROR_GENERIC,
                                     _("Empty matches cannot be written as"
                                       "SGF"));
                return FALSE;
        }

        flavor = gsgf_flavor_backgammon_new ();

        collection = gsgf_collection_new ();
        for (game_number = 0; ; ++game_number) {
                game = gibbon_match_get_nth_game (match, game_number);
                if (!game)
                        break;
                game_tree = gsgf_collection_add_game_tree (collection, flavor);
                if (!game_tree) {
                        g_object_unref (collection);
                        return FALSE;
                }
                if (!gibbon_sgf_writer_write_game (self, game_tree, game,
                                                   game_number, match, error)) {
                        g_object_unref (collection);
                        return FALSE;
                }
        }

        return gsgf_component_write_stream (GSGF_COMPONENT (collection),
                                            out, &bytes_written, NULL, error);
}

/*
 * Note that we swap black and white on output so that Gibbon's notion of black
 * and white matches that of GNU Backgammon.
 */
static gboolean
gibbon_sgf_writer_write_game (const GibbonSGFWriter *self,
                              GSGFGameTree *game_tree,
                              const GibbonGame *game, guint game_number,
                              const GibbonMatch *match,
                              GError **error)
{
        GSGFNode *root;
        const gchar *text;
        gchar *str;
        GSGFValue *simple_text;
        GSGFValue *match_info;
        GSGFCookedValue *mi_key, *mi_value;
        GSGFCookedValue *mi_compose;
        const GSGFFlavor *flavor;
        const GibbonPosition *pos;
        gint score;
        GSGFValue *result = NULL;
        GSGFResultCause cause;
        glong action_num;
        GibbonPositionSide side;
        const GibbonGameAction *action = NULL;
        const GibbonPosition *position;

        if (!gsgf_game_tree_set_application (game_tree,
                                             PACKAGE, VERSION,
                                             error))
                return FALSE;

        root = gsgf_game_tree_add_node (game_tree);

        text = gibbon_match_get_black (match);
        if (!text)
                text = "white";
        simple_text = GSGF_VALUE (gsgf_simple_text_new (text));
        if (!gsgf_node_set_property (root, "PW", simple_text, error)) {
                g_object_unref (simple_text);
                return FALSE;
        }

        text = gibbon_match_get_white (match);
        if (!text)
                text = "black";
        simple_text = GSGF_VALUE (gsgf_simple_text_new (text));
        if (!gsgf_node_set_property (root, "PB", simple_text, error)) {
                g_object_unref (simple_text);
                return FALSE;
        }

        if (gibbon_match_get_crawford (match)) {
                text = gibbon_game_is_crawford (game) ?
                                "Crawford:CrawfordGame" : "Crawford";
                 simple_text = GSGF_VALUE (gsgf_simple_text_new (text));
                 if (!gsgf_node_set_property (root, "RU", simple_text, error)) {
                         g_object_unref (simple_text);
                         return FALSE;
                 }
        }

        flavor = gsgf_game_tree_get_flavor (game_tree);
        match_info = GSGF_VALUE (gsgf_list_of_new (gsgf_compose_get_type (),
                                                   flavor));
        if (!gsgf_node_set_property (root, "MI", match_info, error)) {
                g_object_unref (match_info);
                return FALSE;
        }

        mi_key = GSGF_COOKED_VALUE (gsgf_simple_text_new ("length"));
        str = g_strdup_printf ("%u", (guint) gibbon_match_get_length (match));
        mi_value = GSGF_COOKED_VALUE (gsgf_simple_text_new (str));
        g_free (str);
        mi_compose = GSGF_COOKED_VALUE (gsgf_compose_new (mi_key, mi_value,
                                                          NULL));
        if (!gsgf_list_of_append (GSGF_LIST_OF (match_info),
                                  mi_compose, error)) {
                g_object_unref (mi_key);
                g_object_unref (mi_value);
                g_object_unref (mi_compose);
                return FALSE;
        }

        mi_key = GSGF_COOKED_VALUE (gsgf_simple_text_new ("game"));
        str = g_strdup_printf ("%u", game_number);
        mi_value = GSGF_COOKED_VALUE (gsgf_simple_text_new (str));
        g_free (str);
        mi_compose = GSGF_COOKED_VALUE (gsgf_compose_new (mi_key, mi_value,
                                                          NULL));
        if (!gsgf_list_of_append (GSGF_LIST_OF (match_info),
                                  mi_compose, error)) {
                g_object_unref (mi_key);
                g_object_unref (mi_value);
                g_object_unref (mi_compose);
                return FALSE;
        }

        pos = gibbon_game_get_nth_position (game, 0);

        mi_key = GSGF_COOKED_VALUE (gsgf_simple_text_new ("ws"));
        str = g_strdup_printf ("%u", pos->scores[1]);
        mi_value = GSGF_COOKED_VALUE (gsgf_simple_text_new (str));
        g_free (str);
        mi_compose = GSGF_COOKED_VALUE (gsgf_compose_new (mi_key, mi_value,
                                                          NULL));
        if (!gsgf_list_of_append (GSGF_LIST_OF (match_info),
                                  mi_compose, error)) {
                g_object_unref (mi_key);
                g_object_unref (mi_value);
                g_object_unref (mi_compose);
                return FALSE;
        }

        mi_key = GSGF_COOKED_VALUE (gsgf_simple_text_new ("bs"));
        str = g_strdup_printf ("%u", pos->scores[0]);
        mi_value = GSGF_COOKED_VALUE (gsgf_simple_text_new (str));
        g_free (str);
        mi_compose = GSGF_COOKED_VALUE (gsgf_compose_new (mi_key, mi_value,
                                                          NULL));
        if (!gsgf_list_of_append (GSGF_LIST_OF (match_info),
                                  mi_compose, error)) {
                g_object_unref (mi_key);
                g_object_unref (mi_value);
                g_object_unref (mi_compose);
                return FALSE;
        }

        score = gibbon_game_over (game);
        cause = gibbon_game_resignation (game)
                        ? GSGF_RESULT_RESIGNATION : GSGF_RESULT_NORMAL;
        if (score > 0)
                result = GSGF_VALUE (gsgf_result_new (GSGF_RESULT_BLACK, score,
                                                      cause));
        else if (score < 0)
                result = GSGF_VALUE (gsgf_result_new (GSGF_RESULT_WHITE, -score,
                                                      cause));
        if (result
            && !gsgf_node_set_property (root, "RE", result, error)) {
                        g_object_unref (result);
                        return FALSE;
        }

        for (action_num = 0; ; ++action_num) {
                action = gibbon_game_get_nth_action (game, action_num, &side);
                if (!action)
                        break;
                if (action_num)
                        position = gibbon_game_get_nth_position (game,
                                                                 action_num
                                                                 - 1);
                if (GIBBON_IS_ROLL (action)) {
                        if (!side)
                                continue;
                        if (!gibbon_sgf_writer_roll (self, game_tree, side,
                                                     GIBBON_ROLL (action),
                                                     error))
                                return FALSE;
                } else if (GIBBON_IS_MOVE (action)) {
                        if (!side)
                                continue;
                        if (!gibbon_sgf_writer_move (self, game_tree, side,
                                                     GIBBON_MOVE (action),
                                                     error))
                                return FALSE;
                } else if (GIBBON_IS_DOUBLE (action)) {
                        if (!side)
                                continue;
                        if (!gibbon_sgf_writer_special_move (self, game_tree,
                                                             side, "double",
                                                             error))
                                return FALSE;
                } else if (GIBBON_IS_TAKE (action)) {
                        if (!side)
                                continue;
                        if (!gibbon_sgf_writer_special_move (self, game_tree,
                                                             side, "take",
                                                             error))
                                return FALSE;
                } else if (GIBBON_IS_DROP (action)) {
                        if (!side)
                                continue;
                        if (!gibbon_sgf_writer_special_move (self, game_tree,
                                                             side, "drop",
                                                             error))
                                return FALSE;
                }
        }

        return TRUE;
}

static
gboolean gibbon_sgf_writer_roll (const GibbonSGFWriter *self,
                                 GSGFGameTree *game_tree,
                                 GibbonPositionSide side,
                                 GibbonRoll *roll, GError **error)
{
        gchar *buffer;
        GSGFValue *simple_text;
        GSGFNode *node;

        buffer = g_strdup_printf ("%d%d", roll->die1, roll->die2);
        simple_text = GSGF_VALUE (gsgf_simple_text_new (buffer));
        g_free (buffer);

        node = gsgf_game_tree_add_node (game_tree);
        if (!gsgf_node_set_property (node, "DI", simple_text, error)) {
                g_object_unref (simple_text);
                return FALSE;
        }

        if (side == GIBBON_POSITION_SIDE_WHITE)
                simple_text = GSGF_VALUE (gsgf_simple_text_new ("Black"));
        else
                simple_text = GSGF_VALUE (gsgf_simple_text_new ("White"));
        if (!gsgf_node_set_property (node, "PL", simple_text, error)) {
                g_object_unref (simple_text);
                return FALSE;
        }

        return TRUE;
}

static
gboolean gibbon_sgf_writer_move (const GibbonSGFWriter *self,
                                 GSGFGameTree *game_tree,
                                 GibbonPositionSide side,
                                 GibbonMove *move, GError **error)
{
        GList *last_element;
        GSGFNode *node = NULL;
        const gchar *side_str;
        GSGFMoveBackgammon *gsgf_move;

        gibbon_match_return_val_if_fail (move->number < 5, FALSE, error);

        last_element = gsgf_game_tree_get_last_node (game_tree);
        if (last_element) {
                node = last_element->data;
                if (gsgf_node_get_property (node, "DI")
                    && gsgf_node_get_property (node, "PL")) {
                        gsgf_node_remove_property (node, "DI");
                        gsgf_node_remove_property (node, "PL");
                } else {
                        node = NULL;
                }
        }

        if (!node)
                node = gsgf_game_tree_add_node (game_tree);

        switch (move->number) {
        case 0:
                gsgf_move = gsgf_move_backgammon_new_regular (move->die1,
                                                              move->die2,
                                                              error, -1);
                break;
        case 1:
                gsgf_move = gsgf_move_backgammon_new_regular (
                                move->die1, move->die2, error,
                                translate_point (move->movements[0].from,
                                                 side),
                                translate_point (move->movements[0].to,
                                                side),
                                -1);
                break;
        case 2:
                gsgf_move = gsgf_move_backgammon_new_regular (
                                move->die1, move->die2, error,
                                translate_point (move->movements[0].from,
                                                 side),
                                translate_point (move->movements[0].to,
                                                 side),
                                translate_point (move->movements[1].from,
                                                 side),
                                translate_point (move->movements[1].to,
                                                 side),
                                -1);
                break;
        case 3:
                gsgf_move = gsgf_move_backgammon_new_regular (
                                move->die1, move->die2, error,
                                translate_point (move->movements[0].from,
                                                 side),
                                translate_point (move->movements[0].to,
                                                 side),
                                translate_point (move->movements[1].from,
                                                 side),
                                translate_point (move->movements[1].to,
                                                 side),
                                translate_point (move->movements[2].from,
                                                 side),
                                translate_point (move->movements[2].to,
                                                 side),
                                -1);
                break;
        case 4:
                gsgf_move = gsgf_move_backgammon_new_regular (
                                move->die1, move->die2, error,
                                translate_point (move->movements[0].from,
                                                 side),
                                translate_point (move->movements[0].to,
                                                 side),
                                translate_point (move->movements[1].from,
                                                 side),
                                translate_point (move->movements[1].to,
                                                 side),
                                translate_point (move->movements[2].from,
                                                 side),
                                translate_point (move->movements[2].to,
                                                 side),
                                translate_point (move->movements[3].from,
                                                 side),
                                translate_point (move->movements[3].to,
                                                 side),
                                -1);
                break;
        default:
                return FALSE; /* NOT_REACHED */
        }
        if (!gsgf_move)
                return FALSE;

        side_str = side == GIBBON_POSITION_SIDE_WHITE ? "B" : "W";
        if (!gsgf_node_set_property (node, side_str, GSGF_VALUE (gsgf_move),
                                     error)) {
                g_object_unref (gsgf_move);
                return FALSE;
        }

        return TRUE;
}

static
gboolean gibbon_sgf_writer_special_move (const GibbonSGFWriter *self,
                                         GSGFGameTree *game_tree,
                                         GibbonPositionSide side,
                                         const gchar *special, GError **error)
{
        GSGFNode *node = NULL;
        const gchar *side_str;
        GSGFMoveBackgammon *gsgf_move;

        gsgf_move = gsgf_move_backgammon_new_from_string (special, error);
        if (!gsgf_move)
                return FALSE;

        node = gsgf_game_tree_add_node (game_tree);
        side_str = side == GIBBON_POSITION_SIDE_WHITE ? "B" : "W";
        if (!gsgf_node_set_property (node, side_str, GSGF_VALUE (gsgf_move),
                                     error)) {
                g_object_unref (gsgf_move);
                return FALSE;
        }

        return TRUE;
}

static guint
translate_point (guint point, GibbonPositionSide side)
{
        if (point == 0) {
                if (side == GIBBON_POSITION_SIDE_BLACK)
                        return 24;
                else
                        return 25;
        }

        /* Swap colors for SGF export! */
        if (point <= 24)
                return 25 - point - 1;

        if (side == GIBBON_POSITION_SIDE_BLACK)
                return 25;
        else
                return 24;
}
