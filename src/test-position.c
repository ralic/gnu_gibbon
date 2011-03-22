/*
 * This file is part of Gibbon, a graphical frontend to the First Internet 
 * Backgammon Server FIBS.
 * Copyright (C) 2009-2011 Guido Flohr, http://guido-flohr.net/.
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

#include <string.h>

#include <glib.h>

#include <gibbon-position.h>

static gboolean test_constructor (void);
static gboolean test_copy_constructor (void);

int
main(int argc, char *argv[])
{
	int status = 0;

        g_type_init ();

        if (!test_constructor ())
                status = -1;

        if (!test_copy_constructor ())
                status = -1;

        return status;
}

static gboolean
test_constructor ()
{
        GibbonPosition *pos = gibbon_position_new ();

        g_return_val_if_fail (pos != NULL, FALSE);

        g_return_val_if_fail (pos->players[0] == NULL, FALSE);
        g_return_val_if_fail (pos->players[1] == NULL, FALSE);

        g_return_val_if_fail (pos->points[0] == -2, FALSE);
        g_return_val_if_fail (pos->points[1] == 0, FALSE);
        g_return_val_if_fail (pos->points[2] == 0, FALSE);
        g_return_val_if_fail (pos->points[3] == 0, FALSE);
        g_return_val_if_fail (pos->points[4] == 0, FALSE);
        g_return_val_if_fail (pos->points[5] == 5, FALSE);
        g_return_val_if_fail (pos->points[6] == 0, FALSE);
        g_return_val_if_fail (pos->points[7] == 3, FALSE);
        g_return_val_if_fail (pos->points[8] == 0, FALSE);
        g_return_val_if_fail (pos->points[9] == 0, FALSE);
        g_return_val_if_fail (pos->points[10] == 0, FALSE);
        g_return_val_if_fail (pos->points[11] == -5, FALSE);
        g_return_val_if_fail (pos->points[12] == 5, FALSE);
        g_return_val_if_fail (pos->points[13] == 0, FALSE);
        g_return_val_if_fail (pos->points[14] == 0, FALSE);
        g_return_val_if_fail (pos->points[15] == 0, FALSE);
        g_return_val_if_fail (pos->points[16] == -3, FALSE);
        g_return_val_if_fail (pos->points[17] == 0, FALSE);
        g_return_val_if_fail (pos->points[18] == -5, FALSE);
        g_return_val_if_fail (pos->points[19] == 0, FALSE);
        g_return_val_if_fail (pos->points[20] == 0, FALSE);
        g_return_val_if_fail (pos->points[21] == 0, FALSE);
        g_return_val_if_fail (pos->points[22] == 0, FALSE);
        g_return_val_if_fail (pos->points[23] == 2, FALSE);

        g_return_val_if_fail (pos->bar[0] == 0, FALSE);
        g_return_val_if_fail (pos->bar[1] == 0, FALSE);

        g_return_val_if_fail (pos->dice[0] == 0, FALSE);
        g_return_val_if_fail (pos->dice[1] == 0, FALSE);

        g_return_val_if_fail (pos->cube == 0, FALSE);
        g_return_val_if_fail (pos->may_double == FALSE, FALSE);

        gibbon_position_free (pos);

        return TRUE;
}

static gboolean
test_copy_constructor (void)
{
        GibbonPosition *orig = gibbon_position_new ();
        GibbonPosition *copy;
        gchar *player1;
        gchar *player2;

        orig->players[0] = g_strdup ("foo");
        orig->players[1] = g_strdup ("bar");

        copy = gibbon_position_copy (orig);
        g_return_val_if_fail (copy != NULL, FALSE);

        g_return_val_if_fail (orig->players[0] != copy->players[0], FALSE);
        g_return_val_if_fail (orig->players[1] != copy->players[1], FALSE);

        g_return_val_if_fail (!g_strcmp0 (orig->players[0],
                                          copy->players[0]), FALSE);
        g_return_val_if_fail (!g_strcmp0 (orig->players[1],
                                          copy->players[1]), FALSE);

        /* Compare the structure expect for the pointers.  */
        player1 = copy->players[0];
        player2 = copy->players[1];

        copy->players[0] = orig->players[0];
        copy->players[1] = orig->players[1];

        g_return_val_if_fail (!memcmp (orig, copy, sizeof *orig), FALSE);

        copy->players[0] = player1;
        copy->players[1] = player2;

        gibbon_position_free (orig);
        gibbon_position_free (copy);

        return TRUE;
}