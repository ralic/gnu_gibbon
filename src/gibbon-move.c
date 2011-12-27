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
 * SECTION:gibbon-move
 * @short_description: A backgammon move in FIBS
 *
 * Since: 0.1.1
 *
 * A complete backgammon move.
 */

#include <glib.h>
#include <glib/gi18n.h>

#include "gibbon-move.h"

G_DEFINE_TYPE (GibbonMove, gibbon_move, GIBBON_TYPE_GAME_ACTION)

static void 
gibbon_move_init (GibbonMove *self)
{
        self->movements = NULL;
}

static void
gibbon_move_finalize (GObject *object)
{
        GibbonMove *self = GIBBON_MOVE (object);

        g_free (self->movements);

        G_OBJECT_CLASS (gibbon_move_parent_class)->finalize(object);
}

static void
gibbon_move_class_init (GibbonMoveClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gibbon_move_finalize;
}

GibbonMove *
gibbon_move_new (gint die1, gint die2, gsize num_movements)
{
        GibbonMove *self = g_object_new (GIBBON_TYPE_MOVE, NULL);

        self->die1 = die1;
        self->die2 = die2;
        self->status = GIBBON_MOVE_LEGAL;
        self->number = 0;
        self->movements = g_malloc0 (num_movements * sizeof *self->movements);

        return self;
}

GibbonMove *
gibbon_move_newv (gint die1, gint die2, ...)
{
        GibbonMove *self = gibbon_move_new (die1, die2, 4);
        GibbonMovement *movement;
        va_list ap;
        gint i, from, to;

        va_start (ap, die2);
        for (i = 0; i < 4; ++i) {
                from = va_arg (ap, gint);
                if (from < 0)
                        break;
                if (from > 24) {
                        va_end (ap);
                        g_critical ("gibbon_move_newv: starting point %d"
                                    " is out of range!", from);
                        return self;
                }
                to = va_arg (ap, gint);
                if (to < 0) {
                        va_end (ap);
                        g_critical ("gibbon_move_newv: odd number of points!");
                        return self;
                } else if (to < 1 || to > 25) {
                        va_end (ap);
                        g_critical ("gibbon_move_newv: end point %d"
                                    " is out of range!", to);
                        return self;
                }
                self->number++;
                movement = self->movements + i;
                movement->from = from;
                movement->to = to;
                /*
                 * We do not bother to find out, which die was used.
                 */
                movement->die = -1;
        }
        va_end (ap);

        return self;
}

GibbonMove *
gibbon_move_copy (const GibbonMove *self)
{
        GibbonMove *copy;
        GibbonMovement *src, *dest;
        gsize i;

        g_return_val_if_fail (GIBBON_IS_MOVE (self), NULL);

        copy = gibbon_move_new (self->die1, self->die2, self->number);

        /*
         * We cannot use gibbon_movement_copy here because that would produce
         * four pointers, where we need just one.
         */
        for (i = 0; i < self->number; ++i) {
                src = self->movements + i;
                dest = copy->movements + i;
                dest->from = src->from;
                dest->to = src->to;
                dest->die = src->die;
        }

        return copy;
}
