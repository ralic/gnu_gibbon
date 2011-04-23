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

/* Move patterns describe how a double roll was used, when we know the set
 * of starting points.
 *
 * For decoding, they have to be split into bytes first.  Each byte is then
 * split into nibbles.  The left nibble gives the index into the set of
 * starting points, and the right number gives the number of dice values
 * to use.
 *
 * For example, if we know that the user has moved checkers from points
 * 6, 9, and 20, and has rolled a double 2, a 0x12 means that he has
 * moved two checkers from the 9-point (9 is point number 1 in the set
 * 6, 9, and 20).
 */
guint move_patterns1[] = {
                0x0103
};

guint move_patterns2[] = {
                0x1202,
                0x1103,
                0x110201,
                0x111201,
                0x111102,
                0x11010101,
                0x111101
};

guint move_patterns3[] = {
                0x211102,
                0x21110101,
                0x21111101,
                0x21211101
};

guint move_patterns4[] = {
                0x31211101
};

#if (0)
static void dump_move (const GibbonMove *move);
#endif

static void gibbon_position_fill_movement (GibbonMove *move,
                                           guint point, guint die);
static GList *gibbon_position_find_double (const gint *before,
                                           const gint *after,
                                           guint die,
                                           gsize num_froms,
                                           const guint *froms);
static GList *gibbon_position_find_non_double (const gint *before,
                                               const gint *after,
                                               guint die1, guint die2,
                                               gsize num_froms,
                                               const guint *froms);
static gboolean gibbon_position_is_diff (const gint before[26],
                                         const gint after[26],
                                         GibbonMove *move);
static gboolean gibbon_position_can_move (const gint board[26], gint die);
static gint find_backmost_checker (const gint board[26]);

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
        gint checkers = 15;
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

        if (checkers < 0)
                checkers = 0;

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

GibbonMove *
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
        guint die1, die2, other_die;
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
        if (side == GIBBON_POSITION_SIDE_WHITE) {
                before[0] = _before->bar[1];
                after[0] = _after->bar[1];
                before[25] = _before->bar[0];
                after[25] = _after->bar[0];
        } else {
                /* "Normalize" the board representation.  Negative
                 * checker counts are ours, positive ones are hers.
                 */
                before[0] = _before->bar[0];
                after[0] = _after->bar[0];
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
                before[25] = _before->bar[1];
                after[25] = _after->bar[1];
        }

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

        if (die1 != die2) {
                if (move->number == 0) {
                        if (gibbon_position_can_move (after, die1)
                            || gibbon_position_can_move (after, die2)) {
                                move->status = GIBBON_MOVE_USE_ALL;
                                return move;
                        }
                } else if (move->number == 1) {
                        if (move->movements[0].die == die1)
                                other_die = die2;
                        else
                                other_die = die1;
                        if (gibbon_position_can_move (after, other_die)) {
                                move->status = GIBBON_MOVE_USE_ALL;
                                return move;
                        }
                }
        }

        return move;
}

static gboolean
gibbon_position_can_move (const gint board[26], gint die)
{
        gint i;
        gint backmost;

        /* Dance? */
        if (board[25]) {
                if (board[25 - die] < -1)
                        return FALSE;
                else
                        return TRUE;
        }

        /* Move freely?  */
        for (i = 24; i > die; --i)
                if (board[i] >= 1 && board[i - die] >= -1)
                        return TRUE;

        /* Can we bear-off? */
        backmost = find_backmost_checker (board);
        if (backmost > 6)
                return FALSE;

        /* Direct bear-off? */
        if (board[die] >= 1)
                return TRUE;

        /* Wasteful bear-off? */
        if (die > backmost)
                return TRUE;

        return FALSE;
}

static gboolean
gibbon_position_is_diff (const gint _before[26], const gint after[26],
                         GibbonMove *move)
{
        gint before[26];
        gint i, from, to;
        const GibbonMovement *movement;
        gint backmost;

        /* dump_move (move); */
        memcpy (before, _before, sizeof before);

        for (i = 0; i < move->number; ++i) {
                movement = move->movements + i;

                from = movement->from;
                if (from <= 0) {
                        /* Actually this move is a duplicate.  */
                        move->status = GIBBON_MOVE_ILLEGAL;
                        return FALSE;
                }

                to = movement->to;

                if (to == 0) {
                        backmost = find_backmost_checker (before);

                        if (backmost > 6)
                                move->status = GIBBON_MOVE_PREMATURE_BEAR_OFF;
                        /* Although this move is illegal, we continue.  If
                         * the resulting position matches, we know that the
                         * intentionally bore off the checker, and we can
                         * inform about the error.
                         */
                } else if (before[to] < -1) {
                        /* Is the target point occupied?  */
                        move->status = GIBBON_MOVE_BLOCKED;
                        return FALSE;
                } else if (before[to] == -1) {
                        before[to] = 0;
                        ++before[0];
                }

                if (to)
                        ++before[to];
                /* Else: No need to know about it.  We count the checkers on
                 * the board instead, and calculate the difference.
                 */
                --before[from];
        }

        /* Is the resulting position identical to what we expect? */
        if (memcmp (before, after, sizeof before)) {
                for (i = 25; i >= 0; --i) {
                        g_printerr ("  %d: %d | %d\n", i, before[i], after[i]);
                }
        }
        if (memcmp (before, after, sizeof before))
                return FALSE;

        /* At this point we know:
         *
         * 1) All intermediate points and the landing point are free.
         * 2) The resulting position exactly reflects the effect of the move.
         *
         * After this, we only have to check these constraints:
         *
         * - No legal moves, while checkers on the bar.
         * - Maximize number of pips used.
         */

        return TRUE;
}

static void
gibbon_position_fill_movement (GibbonMove *move, guint point, guint die)
{
        guint movement_num = move->number;

        move->movements[movement_num].from = point;
        move->movements[movement_num].to = point - die;
        if (move->movements[movement_num].to < 0)
                move->movements[movement_num].to = 0;
        move->movements[movement_num].die = die;
        ++move->number;
}

static GList *
gibbon_position_find_non_double (const gint *before,
                                 const gint *after,
                                 guint die1, guint die2,
                                 gsize num_froms, const guint *froms)
{
        GibbonMove *move;
        GList *moves = NULL;

        if (!num_froms) {
                move = gibbon_position_alloc_move (0);
                moves = g_list_append (moves, move);

                return moves;
        }

        /* Two possibilities.  */
        if (2 == num_froms) {
                move = gibbon_position_alloc_move (2);
                gibbon_position_fill_movement (move, froms[0], die1);
                gibbon_position_fill_movement (move, froms[1], die2);
                moves = g_list_append (moves, move);

                move = gibbon_position_alloc_move (2);
                gibbon_position_fill_movement (move, froms[0], die2);
                gibbon_position_fill_movement (move, froms[1], die1);
                moves = g_list_append (moves, move);

                return moves;
        }

        /* Only one checker was moved.  This can happen in five distinct
         * ways.
         */
        if (froms[0] > die1) {
                move = gibbon_position_alloc_move (2);
                gibbon_position_fill_movement (move, froms[0], die1);
                gibbon_position_fill_movement (move, froms[0] - die1, die2);
                moves = g_list_append (moves, move);
        }
        if (froms[0] > die2) {
                move = gibbon_position_alloc_move (2);
                gibbon_position_fill_movement (move, froms[0], die2);
                gibbon_position_fill_movement (move, froms[0] - die2, die1);
                moves = g_list_append (moves, move);
        }

        move = gibbon_position_alloc_move (1);
        gibbon_position_fill_movement (move, froms[0], die1);
        moves = g_list_append (moves, move);

        move = gibbon_position_alloc_move (1);
        gibbon_position_fill_movement (move, froms[0], die2);
        moves = g_list_append (moves, move);

        move = gibbon_position_alloc_move (2);
        gibbon_position_fill_movement (move, froms[0], die1);
        gibbon_position_fill_movement (move, froms[0], die2);
        moves = g_list_append (moves, move);

        return moves;
}

static GList *
gibbon_position_find_double (const gint *before,
                             const gint *after,
                             guint die,
                             gsize num_froms, const guint *froms)
{
        guint *move_patterns;
        GList *moves = NULL;
        gsize i, j, num_patterns;
        GibbonMove *move;
        gsize num_steps;
        guint pattern;
        guint from_index;
        gint from;

        switch (num_froms) {
                case 0:
                        move = gibbon_position_alloc_move (0);
                        moves = g_list_append (moves, move);

                        return moves;
                case 1:
                        move_patterns = move_patterns1;
                        num_patterns = (sizeof move_patterns1)
                                        / sizeof *move_patterns1;
                        break;
                case 2:
                        move_patterns = move_patterns2;
                        num_patterns = (sizeof move_patterns2)
                                        / sizeof *move_patterns2;
                        break;
                case 3:
                        move_patterns = move_patterns3;
                        num_patterns = (sizeof move_patterns3)
                                        / sizeof *move_patterns3;
                        break;
                default:
                        move_patterns = move_patterns4;
                        num_patterns = (sizeof move_patterns4)
                                        / sizeof *move_patterns4;
                        break;
        }

        for (i = 0; i < num_patterns; ++i) {
                pattern = move_patterns[i];
                /* This may allocate too much but calculating the correct size
                 * would never pay out.
                 */
                move = gibbon_position_alloc_move (4);

                while (pattern) {
                        num_steps = pattern & 0xf;
                        from_index = (pattern & 0xf0) >> 4;
                        from = froms[from_index];
                        for (j = 0; j < num_steps; ++j) {
                                gibbon_position_fill_movement (move, from, die);
                                from -= die;
                        }

                        /* FIXME! The sub moves have to be sorted.  Otherwise,
                         * the correct order for bearing off could not
                         * be found.
                         */
                        pattern >>= 8;
                }
                moves = g_list_append (moves, move);
        }

        return moves;
}

#if (0)
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
        }

        g_printerr ("\n");
}
#endif

static gint
find_backmost_checker (const gint board[26])
{
        gint i;

        for (i = 25; i > 0; --i)
                if (board[i] > 0)
                        break;

        if (!i)
                return 26;

        return i;
}
