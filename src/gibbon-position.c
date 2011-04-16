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

/**
 * SECTION:gibbon-position
 * @short_description: Boxed type for a position in backgammon.
 *
 * Since: 0.1.1
 *
 * This structure holds all information needed to display a
 * normalized backgammon position.
 *
 * If X is black, and O is white, the normalized starting position of a
 * game of backgammon looks like this:
 *
 * <programlisting>
 *  +13-14-15-16-17-18-------19-20-21-22-23-24-+ X: black
 *  | O           X    |   |  X              O |
 *  | O           X    |   |  X              O |
 *  | O           X    |   |  X                |
 *  | O                |   |  X                |
 *  | O                |   |  X                |
 * v|                  |BAR|                   |
 *  | X                |   |  O                |
 *  | X                |   |  O                |
 *  | X           O    |   |  O                |
 *  | X           O    |   |  O              X |
 *  | X           O    |   |  O              X |
 *  +12-11-10--9--8--7--------6--5--4--3--2--1-+ O: white
 * </programlisting>
 *
 * Translated into the checkers array of a #GibbonPosition the image
 * looks like this:
 *
 * <programlisting>
 *  +12-13-14-15-16-17-------18-19-20-21-22-23-+ negative: black or X
 *  |+5          -3    |   | -5             +2 |
 * v|                  |BAR|                   |
 *  |-5          +3    |   | +5             -2 |
 *  +11-10--9--8--7--6--------5--4--3--2--1--0-+ positive: white or O
 * </programlisting>
 */

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "gibbon-position.h"

G_DEFINE_BOXED_TYPE (GibbonPosition, gibbon_position,            \
                     gibbon_position_copy, gibbon_position_free)

GibbonPosition initial = {
                { NULL, NULL },
                0,
                { 0, 0 },
                { 0, 0, 0, 0, 0, 0 },
                { -2,  0,  0,  0,  0,  5,  0,  3,  0,  0,  0, -5,
                   5,  0,  0,  0, -3,  0, -5,  0,  0,  0,  0,  2 },
                { 0, 0, 0, 0, 0, 0 },
                { 0, 0 },
                { 0, 0 },
                1,
                { FALSE, FALSE },
                NULL
};

static void dump_move (const GibbonMove *move);

static GibbonMove *gibbon_position_alloc_move (gsize num_movements);
static GibbonMove *gibbon_position_copy_move (const GibbonMove *src);
static void gibbon_position_fill_movement (GibbonMove *move,
                                           guint point, guint die,
                                           guint num_checkers);
static GList *gibbon_position_find_double (const gint *before,
                                           const gint *after,
                                           guint die,
                                           gsize num_froms,
                                           const guint *froms);
static GList *gibbon_position_find_double4 (const gint *before,
                                            const gint *after,
                                            guint die,
                                            const guint *froms);
static GList *gibbon_position_find_double3 (const gint *before,
                                            const gint *after,
                                            guint die,
                                            const guint *froms);
static GList *gibbon_position_find_non_double (const gint *before,
                                               const gint *after,
                                               guint die1, guint die2,
                                               gsize num_froms,
                                               const guint *froms);
static gboolean gibbon_position_is_diff (const gint *before,
                                         const gint *after,
                                         GibbonMove *move);

/**
 * gibbon_position_new:
 *
 * Creates a new #GibbonPosition, set to the backgammon starting position.
 * Both player names are %NULL, the cube is at 0, and both dice are set
 * to 0.  The @may_double flag is %FALSE.
 *
 * Returns: The newly created #GibbonPosition or %NULL in case of failure.
 */
GibbonPosition *
gibbon_position_new (void)
{
        GibbonPosition *self = g_malloc (sizeof initial);

        *self = initial;

        return self;
}

/**
 * gibbon_position_free:
 *
 * Free all resources associated with the #GibbonPosition.  Note that this
 * function calls g_free() with the player names if not %NULL.
 */
void
gibbon_position_free (GibbonPosition *self)
{
        if (self) {
                if (self->players[0])
                        g_free (self->players[0]);
                if (self->players[1])
                        g_free (self->players[1]);
                if (self->game_info)
                        g_free (self->game_info);
                g_free (self);
        }
}

/**
 * gibbon_position_copy:
 * @self: the original #GibbonPosition.
 *
 * Creates an exact copy of @self.  If player names were set, they are
 * identical in the copy but will use new buffers that can be freed with
 * g_free().
 *
 * Returns: The copied #GibbonPosition or %NULL if @self was %NULL;
 */
GibbonPosition *
gibbon_position_copy (const GibbonPosition *self)
{
        GibbonPosition *copy;

        g_return_val_if_fail (self != NULL, NULL);

        copy = g_malloc (sizeof *self);
        *copy = *self;
        copy->players[0] = g_strdup (copy->players[0]);
        copy->players[1] = g_strdup (copy->players[1]);
        copy->game_info = g_strdup (copy->game_info);

        return copy;
}

guint
gibbon_position_get_borne_off (const GibbonPosition *self,
                               GibbonPositionSide side)
{
        guint checkers = 15;
        guint i;

        if (!side)
                return 0;

        for (i = 0; i < 24; ++i) {
                if (side < 0 && self->points[i] < 0)
                        checkers += self->points[i];
                else if (side > 0 && self->points[i] > 0)
                        checkers -= self->points[i];
        }

        if (side < 0)
                checkers -= abs (self->bar [1]);
        else if (side > 0)
                checkers -= abs (self->bar [0]);

        g_return_val_if_fail (checkers < 15, 0);

        return checkers;
}

guint
gibbon_position_get_pip_count (const GibbonPosition *self,
                               GibbonPositionSide side)
{
        guint pips = 0;
        guint i;

        if (!side)
                return 0;

        if (side < 0) {
                for (i = 0; i < 24; ++i) {
                        if (self->points[i] < 0)
                                pips -= (24 - i) * self->points[i];
                }
                pips += 25 * abs (self->bar[1]);
        } else {
                for (i = 0; i < 24; ++i) {
                        if (self->points[i] > 0)
                                pips += (i + 1) * self->points[i];
                }
                pips += 25 * abs (self->bar[0]);
        }

        return pips;
}

static GibbonMove *
gibbon_position_alloc_move (gsize num_movements)
{
        GibbonMove *move = g_malloc (sizeof move->number
                                     + num_movements * sizeof *move->movements
                                     + sizeof move->status);
        move->number = 0;
        move->status = GIBBON_MOVE_LEGAL;

        return move;
}

GibbonMove *
gibbon_position_check_move (const GibbonPosition *_before,
                            const GibbonPosition *_after,
                            GibbonPositionSide side)
{
        GibbonMove *move;
        GList *found;
        gint before[26];
        gint after[26];
        gint i, tmp;
        guint num_froms = 0;
        guint froms[4];
        guint die1, die2;
        GList *iter;

        die1 = abs (_before->dice[0]);
        die2 = abs (_before->dice[1]);

        move = gibbon_position_alloc_move (0);
        move->status = GIBBON_MOVE_ILLEGAL;

        g_return_val_if_fail (die1 != 0, move);
        g_return_val_if_fail (die2 != 0, move);
        g_return_val_if_fail (side == GIBBON_POSITION_SIDE_WHITE
                              || side == GIBBON_POSITION_SIDE_BLACK,
                              move);

        /* This structure is handier for us.  It would probably be easier
         * if we also change GibbonPosition accordingly.
         */
        memcpy (before + 1, _before->points, 24 * sizeof *before);
        memcpy (after + 1, _after->points, 24 * sizeof *after);
        if (side == GIBBON_POSITION_SIDE_BLACK) {
                /* "Normalize" the board representation.  Negative
                 * checker counts are ours, positive ones are hers.
                 */
                for (i = 1; i <= 24; ++i) {
                        before[i] = -before[i];
                        after[i] = -after[i];
                }

                /* And swap the direction.  */
                for (i = 1; i <= 12; ++i) {
                        tmp = before[25 - i];
                        before[25 - i] = before[i];
                        before[i] = tmp;
                        tmp = after[25 - i];
                        after[25 - i] = after[i];
                        after[i] = tmp;
                }
        }
        before[0] = gibbon_position_get_borne_off (_before, side);
        after[0] = gibbon_position_get_borne_off (_after, side);
        before[25] = _before->bar[0];
        after[25] = _after->bar[0];

        /* Find the number of possible starting points.  */
        for (i = 25; i >= 1; --i) {
                if (after[i] < before[i]) {
                        froms[num_froms++] = i;
                        /* More than four are always illegal.  */
                        if (num_froms > 4) {
                                move->status = GIBBON_MOVE_TOO_MANY_MOVES;
                                return move;
                        }
                }
        }

        /* Find candidate moves.  */
        if (die1 == die2) {
                found = gibbon_position_find_double (before, after,
                                                     die1,
                                                     num_froms, froms);
        } else {
                found = gibbon_position_find_non_double (before, after,
                                                         die1, die2,
                                                         num_froms, froms);
        }

        iter = found;
        while (iter) {
                if (gibbon_position_is_diff (before, after,
                                             (GibbonMove *) iter->data)) {
                        g_free (move);
                        move = (GibbonMove *) iter->data;
                        iter->data = NULL;
                        break;
                }
                iter = iter->next;
        }
        g_list_foreach (found, (GFunc) g_free, NULL);

        if (move->status != GIBBON_MOVE_LEGAL)
                return move;

        return move;
}

static gboolean
gibbon_position_is_diff (const gint *_before, const gint *after,
                         GibbonMove *move)
{
        gint before[26];
        guint i, from, to;
        const GibbonMovement *movement;

        memcpy (before, _before, sizeof before);

        for (i = 0; i < move->number; ++i) {
                movement = move->movements + i;
                to = movement->to;

                /* Is the target point occupied?  */
                if (before[to] < -1) {
                        move->status = GIBBON_MOVE_BLOCKED;
                        return FALSE;
                }

                /* Remove the opponent's checker if this is a hit.  */
                if (before[to] == -1)
                        before[to] = 0;

                before[to] += movement->num;
                from = movement->from;
                before[from] -= movement->num;
        }

        /* Is the resulting position identical to what we expect? */
        if (memcmp (before, after, sizeof before))
                return FALSE;

        /* At this point we know:
         *
         * 1) All intermediate points and the landing point are free.
         * 2) The resulting position exactly reflects the effect of the move.
         *
         * After this, we only have to check these constraints:
         *
         * - Bear-off only allowed, when all checkers are in the home board.
         * - No legal moves, while checkers on the bar.
         * - Maximize number of pips used.
         */
        return TRUE;
}

static void
gibbon_position_fill_movement (GibbonMove *move,
                               guint point, guint die,
                               guint num_checkers)
{
        guint movement_num = move->number;

        move->movements[movement_num].from = point;
        move->movements[movement_num].to = point - die;
        if (move->movements[movement_num].to > 24)
                move->movements[movement_num].to = 24;
        move->movements[movement_num].num = num_checkers;
        ++move->number;
}

static GList *
gibbon_position_find_non_double (const gint *before,
                                 const gint *after,
                                 guint die1, guint die2,
                                 gsize num_froms, const guint *froms)
{
        return NULL;
}

static GList *
gibbon_position_find_double (const gint *before,
                             const gint *after,
                             guint die,
                             gsize num_froms, const guint *froms)
{
        switch (num_froms) {
                case 4:
                        return gibbon_position_find_double4 (before, after,
                                                             die, froms);
                case 3:
                        return gibbon_position_find_double3 (before, after,
                                                             die, froms);
        }

        return NULL;
}

static GList *
gibbon_position_find_double4 (const gint *before,
                              const gint *after,
                              guint die, const guint *froms)
{
        GibbonMove *move = gibbon_position_alloc_move (4);

        gibbon_position_fill_movement (move, froms[0], die, 1);
        gibbon_position_fill_movement (move, froms[1], die, 1);
        gibbon_position_fill_movement (move, froms[2], die, 1);
        gibbon_position_fill_movement (move, froms[3], die, 1);

        return g_list_append (NULL, move);
}

static GList *
gibbon_position_find_double3 (const gint *before,
                              const gint *after,
                              guint die, const guint *froms)
{
        GList *moves = NULL;
        GibbonMove *move;
        GibbonMove *first;
        gint i;

        /* Move each checker once.  */
        move = first = gibbon_position_alloc_move (3);
        gibbon_position_fill_movement (move, froms[0], die, 1);
        gibbon_position_fill_movement (move, froms[1], die, 1);
        gibbon_position_fill_movement (move, froms[2], die, 1);
        moves = g_list_append (moves, move);

        /* Now try to move two checkers or let one of them move twice.  */
        for (i = 0; i < 3; ++i) {
                if (before[froms[i]] > 1) {
                        move = gibbon_position_copy_move (first);
                        move->movements[i].num = 2;
                        moves = g_list_append (moves, move);
                }
        }

        return moves;
}

static GibbonMove *
gibbon_position_copy_move (const GibbonMove *src)
{
        gsize bytes = sizeof src->number
                      + src->number * sizeof *src->movements
                      + sizeof src->status;
        GibbonMove *dest = g_malloc (bytes);

        memcpy (dest, src, bytes);

        return dest;
}

static void
dump_move (const GibbonMove *move)
{
        int i;

        g_printerr ("Move:");
        if (move->status) {
                g_printerr (" error %d.\n", move->status);
                return;
        }

        for (i = 0; i < move->number; ++i) {
                g_printerr (" %u/%u",
                            move->movements[i].from,
                            move->movements[i].to);
                if (move->movements[i].num > 1)
                        g_printerr ("(%u)", move->movements[i].num);
        }

        g_printerr ("\n");
}
