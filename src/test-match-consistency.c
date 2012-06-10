/*
 * This file is part of Gibbon, a graphical frontend to the First Internet 
 * Backgammon Server FIBS.
 * Copyright (C) 2009-2012 Guido Flohr, http://guido-flohr.net/.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>
#include <string.h>

#include <gibbon-match.h>
#include <gibbon-game.h>
#include <gibbon-position.h>
#include <gibbon-move.h>
#include <gibbon-double.h>
#include <gibbon-drop.h>
#include <gibbon-roll.h>
#include <gibbon-take.h>
#include <gibbon-resign.h>
#include <gibbon-reject.h>
#include <gibbon-accept.h>
#include <gibbon-setup.h>

int
main(int argc, char *argv[])
{
	GibbonMatch *match;
	GibbonGameAction *action;
	GError *error = NULL;

        g_type_init ();

        match = gibbon_match_new ("SnowWhite", "JoeBlack", 5, TRUE);
        if (!match) {
                g_printerr ("Match creation failed!\n");
                g_object_unref (match);
                return -1;
        }

        if (!gibbon_match_add_game (match, &error)) {
                g_object_unref (match);
                g_printerr ("Cannot add game: %s!\n", error->message);
                return FALSE;
        }

        action = GIBBON_GAME_ACTION (gibbon_move_newv (3, 1, 8, 5, 6, 5, -1));
        if (gibbon_match_add_action (match, GIBBON_POSITION_SIDE_WHITE, action,
                                     &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Opening move without roll succeded!\n");
                return -1;
        }
        g_error_free (error);
        error = NULL;
        if (gibbon_match_add_action (match, GIBBON_POSITION_SIDE_WHITE, action,
                                     &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Opening move without roll succeded!\n");
                return -1;
        }
        g_error_free (error);
        error = NULL;
        g_object_unref (action);

        action = GIBBON_GAME_ACTION (gibbon_roll_new (3, 1));
        if (!gibbon_match_add_action (match, GIBBON_POSITION_SIDE_NONE, action,
                                      &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Could not add opening roll: %s\n", error->message);
                g_error_free (error);
                return -1;
        }

        action = GIBBON_GAME_ACTION (gibbon_move_newv (3, 1, 17, 21, 19, 21,
                                                       -1));
        if (gibbon_match_add_action (match, GIBBON_POSITION_SIDE_BLACK, action,
                                     &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Opening move succeded but not on move!\n");
                return -1;
        }
        g_error_free (error);
        error = NULL;
        g_object_unref (action);

        action = GIBBON_GAME_ACTION (gibbon_move_newv (3, 1, 8, 5, 6, 5, -1));
        if (!gibbon_match_add_action (match, GIBBON_POSITION_SIDE_WHITE, action,
                                      &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Legal opening move failed: %s\n", error->message);
                g_error_free (error);
                return -1;
        }
        if (gibbon_match_add_action (match, GIBBON_POSITION_SIDE_WHITE, action,
                                     &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Redoing opening move succeded!\n");
                return -1;
        }
        g_error_free (error);
        error = NULL;
        g_object_unref (action);

        action = GIBBON_GAME_ACTION (gibbon_move_newv (3, 1, 17, 21, 19, 21,
                                                       -1));
        if (gibbon_match_add_action (match, GIBBON_POSITION_SIDE_BLACK, action,
                                     &error)) {
                g_object_unref (match);
                g_object_unref (action);
                g_printerr ("Move without roll succeded!\n");
                return -1;
        }
        g_error_free (error);
        error = NULL;
        g_object_unref (action);

        return 0;
}
