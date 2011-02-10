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

#include <glib/gi18n.h>
#include <stdio.h>

#include "test.h"

char *filename = "root-properties.sgf";

static gboolean test_prop_AP(const GSGFNode *node);
static gboolean test_prop_CA(const GSGFNode *node);
static gboolean test_prop_FF(const GSGFNode *node);
static gboolean test_prop_GM(const GSGFNode *node);
static gboolean test_prop_ST(const GSGFNode *node);
static gboolean test_prop_SZ(const GSGFNode *node);
static gboolean test_prop_XY(const GSGFNode *node);

int 
test_collection(GSGFCollection *collection, GError *error)
{
        GList *game_trees;
        GSGFGameTree *game_tree;
        GList *nodes;
        GSGFNode *root_node;

        if (error) {
                fprintf(stderr, "%s: %s\n", filename, error->message);
                return -1;
        }

        game_trees = gsgf_collection_get_game_trees(collection);
        if (!game_trees) {
                fprintf(stderr, "No game trees found.\n");
                return -1;
        }
        game_tree = GSGF_GAME_TREE(game_trees->data);

        nodes = gsgf_game_tree_get_nodes(game_tree);
        if (!nodes) {
                fprintf(stderr, "No nodes found.\n");
                return -1;
        }
        root_node = GSGF_NODE(nodes->data);

        if (!test_prop_AP(root_node))
                return -1;

        if (!test_prop_CA(root_node))
                return -1;

        if (!test_prop_FF(root_node))
                return -1;

        if (!test_prop_GM(root_node))
                return -1;

        if (!test_prop_ST(root_node))
                return -1;

        if (!test_prop_SZ(root_node))
                return -1;

        if (!test_prop_XY(root_node))
                return -1;

        return expect_error(error, NULL);
}

static gboolean
test_prop_AP(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "AP");
        gchar *value;
        gsize length;
        const GSGFCookedValue *subvalue;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'AP'!\n");
                return FALSE;
        }

        if (!GSGF_IS_COMPOSE(cooked_value)) {
                fprintf(stderr, "Root property 'AP' is not a GSGFCompose!\n");
                return FALSE;
        }

        length = gsgf_compose_get_number_of_values(GSGF_COMPOSE(cooked_value));
        if (length != 2) {
                fprintf(stderr, "Property 'AP': Expected 2 subvalues, got %u.\n", length);
                return FALSE;
        }

        subvalue = gsgf_compose_get_value(GSGF_COMPOSE(cooked_value), 0);
        if (!subvalue) {
                fprintf(stderr, "No first 'AP' subvalue!\n");
                return FALSE;
        }
        if (!GSGF_IS_SIMPLE_TEXT(subvalue)) {
                fprintf(stderr, "First 'AP' subvalue is not a GSGFSimpleText!\n");
                return FALSE;
        }
        value = gsgf_text_get_value(GSGF_TEXT(subvalue));
        if (strcmp("foobar", value)) {
                fprintf(stderr, "Expected 'foobar', not '%s'!\n", value);
                return FALSE;
        }

        subvalue = gsgf_compose_get_value(GSGF_COMPOSE(cooked_value), 1);
        if (!subvalue) {
                fprintf(stderr, "No second 'AP' subvalue!\n");
                return FALSE;
        }
        if (!GSGF_IS_SIMPLE_TEXT(subvalue)) {
                fprintf(stderr, "Second 'AP' subvalue is not a GSGFSimpleText!\n");
                return FALSE;
        }
        value = gsgf_text_get_value(GSGF_TEXT(subvalue));
        if (strcmp("1.2.3", value)) {
                fprintf(stderr, "Expected '1.2.3', not '%s'!\n", value);
                return FALSE;
        }

        return TRUE;
}

static gboolean
test_prop_CA(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "CA");
        gchar *value;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'CA'!\n");
                return FALSE;
        }

        if (!GSGF_IS_SIMPLE_TEXT(cooked_value)) {
                fprintf(stderr, "Root property 'CA' is not a GSGFSimpleText!\n");
                return FALSE;
        }

        value = gsgf_text_get_value(GSGF_TEXT(cooked_value));
        if (strcmp ("UTF-8", value)) {
                fprintf(stderr, "CA: Expected 'UTF-8', got '%s'!\n", value);
                return FALSE;
        }

        return TRUE;
}

static gboolean
test_prop_FF(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "FF");
        gint value;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'FF'!\n");
                return FALSE;
        }

        if (!GSGF_IS_NUMBER(cooked_value)) {
                fprintf(stderr, "Root property 'FF' is not a GSGFNumber!\n");
                return FALSE;
        }

        value = gsgf_number_get_value(GSGF_NUMBER(cooked_value));
        if (4 != value) {
                fprintf(stderr, "FF: Expected 4, got %d!\n", value);
                return FALSE;
        }

        return TRUE;
}

static gboolean
test_prop_GM(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "GM");
        gint value;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'GM'!\n");
                return FALSE;
        }

        if (!GSGF_IS_NUMBER(cooked_value)) {
                fprintf(stderr, "Root property 'GM' is not a GSGFNumber!\n");
                return FALSE;
        }

        value = gsgf_number_get_value(GSGF_NUMBER(cooked_value));
        if (6 != value) {
                fprintf(stderr, "GM: Expected 6, got %d!\n", value);
                return FALSE;
        }

        return TRUE;
}

static gboolean
test_prop_ST(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "ST");
        gint value;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'ST'!\n");
                return FALSE;
        }

        if (!GSGF_IS_NUMBER(cooked_value)) {
                fprintf(stderr, "Root property 'ST' is not a GSGFNumber!\n");
                return FALSE;
        }

        value = gsgf_number_get_value(GSGF_NUMBER(cooked_value));
        if (3 != value) {
                fprintf(stderr, "GM: Expected 3, got %d!\n", value);
                return FALSE;
        }

        return TRUE;
}

static gboolean
test_prop_SZ(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "SZ");
        gint64 value;
        gsize length;
        const GSGFCookedValue *subvalue;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'SZ'!\n");
                return FALSE;
        }

        if (!GSGF_IS_COMPOSE(cooked_value)) {
                fprintf(stderr, "Root property 'SZ' is not a GSGFCompose ");
                fprintf(stderr, "but a '%s'.\n", G_OBJECT_TYPE_NAME (cooked_value));
                return FALSE;
        }

        length = gsgf_compose_get_number_of_values(GSGF_COMPOSE(cooked_value));
        if (length != 2) {
                fprintf(stderr, "Property 'SZ': Expected 2 subvalues, got %u.\n", length);
                return FALSE;
        }

        subvalue = gsgf_compose_get_value(GSGF_COMPOSE(cooked_value), 0);
        if (!subvalue) {
                fprintf(stderr, "No first 'SZ' subvalue!\n");
                return FALSE;
        }
        if (!GSGF_IS_NUMBER(subvalue)) {
                fprintf(stderr, "First 'SZ' subvalue is not a GSGFNumber!\n");
                return FALSE;
        }
        value = gsgf_number_get_value(GSGF_NUMBER(subvalue));
        if (24 != value) {
                fprintf(stderr, "Expected 24, not %lld!\n", value);
                return FALSE;
        }

        subvalue = gsgf_compose_get_value(GSGF_COMPOSE(cooked_value), 1);
        if (!subvalue) {
                fprintf(stderr, "No second 'SZ' subvalue!\n");
                return FALSE;
        }
        if (!GSGF_IS_NUMBER(subvalue)) {
                fprintf(stderr, "Second 'SZ' subvalue is not a GSGFNumber!\n");
                return FALSE;
        }
        value = gsgf_number_get_value(GSGF_NUMBER(subvalue));
        if (1 != value) {
                fprintf(stderr, "Expected 1, not %lld!\n", value);
                return FALSE;
        }

        return TRUE;
}

static gboolean
test_prop_XY(const GSGFNode *node)
{
        const GSGFCookedValue *cooked_value = gsgf_node_get_property_cooked(node, "XY");
        const gchar *value;
        gsize length;

        if (!cooked_value) {
                fprintf(stderr, "No root property 'XY'!\n");
                return FALSE;
        }

        if (!GSGF_IS_RAW(cooked_value)) {
                fprintf(stderr, "Root property 'XY' is not a GSGFRaw!\n");
                return FALSE;
        }

        length = gsgf_raw_get_number_of_values(GSGF_RAW(cooked_value));
        if (length != 2) {
                fprintf(stderr, "Expected 2 values for 'XY', got %u.\n", length);
                return FALSE;
        }

        value = gsgf_raw_get_value(GSGF_RAW(cooked_value), 0);
        if (strcmp("Proprietary property #1", value)) {
                fprintf(stderr, "Expected 'Proprietary property #1', got '%s'!\n",
                        value);
                return FALSE;
        }

        return TRUE;
}

