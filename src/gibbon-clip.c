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

/* This is a parser for inbound FIBS messages.  It is _not_
 * complete.  Only the messages that are meaningful to Gibbon are handled,
 * everything else is just passed through.  For example, Gibbon never
 * issues "whois" commands, and therefore does not bother parsing the
 * output of that command.  Instead, it will be just printed to the server
 * console as an unhandled string which is exactly what we want because
 * the command must have been manually sent manually by the user.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include <errno.h>

#include "gibbon-clip.h"
#include "gibbon-util.h"
#include "gibbon-position.h"

#ifdef GIBBON_CLIP_DEBUG_BOARD_STATE
static const gchar *keys[] = {
                "player",
                "opponent",
                "match length",
                "player's score",
                "opponents's score",
                "home/bar",
                "point 0",
                "point 1",
                "point 2",
                "point 3",
                "point 4",
                "point 5",
                "point 6",
                "point 7",
                "point 8",
                "point 9",
                "point 10",
                "point 11",
                "point 12",
                "point 13",
                "point 14",
                "point 15",
                "point 16",
                "point 17",
                "point 18",
                "point 19",
                "point 20",
                "point 21",
                "point 22",
                "point 23",
                "home/bar",
                "turn",
                "player's die 0",
                "player's die 1",
                "opponent's die 0",
                "opponent's die 1",
                "doubling cube",
                "player may double",
                "opponent may double",
                "was doubled",
                "color",
                "direction",
                "home index",
                "bar index",
                "player's checkers on home",
                "opponent's checkers on home",
                "player's checkers on bar",
                "opponent's checkers on bar",
                "can move",
                "forced move",
                "did crawford",
                "redoubles",
                NULL
};

static void gibbon_clip_dump_board (const gchar *raw,
                                    gchar **tokens);
#endif /* #ifdef GIBBON_CLIP_DEBUG_BOARD_STATE */

static gboolean gibbon_clip_parse_clip (const gchar *line,
                                        gchar **tokens,
                                        GSList **result);
static gboolean gibbon_clip_parse_clip_welcome (const gchar *line,
                                                gchar **tokens,
                                                GSList **result);
static gboolean gibbon_clip_parse_clip_own_info (const gchar *line,
                                                 gchar **tokens,
                                                 GSList **result);
static gboolean gibbon_clip_parse_clip_who_info (const gchar *line,
                                                 gchar **tokens,
                                                 GSList **result);
static gboolean gibbon_clip_parse_clip_somebody_message (const gchar *line,
                                                         gchar **tokens,
                                                         GSList **result);
static gboolean gibbon_clip_parse_clip_somebody_something (const gchar *line,
                                                           gchar **tokens,
                                                           GSList **result);
static gboolean gibbon_clip_parse_clip_message (const gchar *line,
                                                gchar **tokens,
                                                GSList **result);
static gboolean gibbon_clip_parse_clip_somebody (const gchar *line,
                                                 gchar **tokens,
                                                 GSList **result);
static gboolean gibbon_clip_parse_clip_something (const gchar *line,
                                                  gchar **tokens,
                                                  GSList **result);

static gboolean gibbon_clip_parse_board (const gchar *line,
                                         gchar **tokens,
                                         GSList **result);

static gboolean gibbon_clip_parse_youre_watching (const gchar *line,
                                                  gchar **tokens,
                                                  GSList **result);

static gboolean gibbon_clip_parse_and (const gchar *line,
                                       gchar **tokens,
                                       GSList **result);
static gboolean gibbon_clip_parse_resuming (const gchar *line,
                                            gchar **tokens,
                                            GSList **result);
static gboolean gibbon_clip_parse_start_match (const gchar *line,
                                               gchar **tokens,
                                               GSList **result);

static gboolean gibbon_clip_parse_wins (const gchar *line,
                                        gchar **tokens,
                                        GSList **result);
static gboolean gibbon_clip_parse_wants (const gchar *line,
                                         gchar **tokens,
                                         GSList **result);

static gboolean gibbon_clip_parse_rolls (const gchar *line,
                                         gchar **tokens,
                                         GSList **result);
static gboolean gibbon_clip_parse_moves (const gchar *line,
                                         gchar **tokens,
                                         GSList **result);

static gboolean gibbon_clip_parse_start_settings (const gchar *line,
                                                  gchar **tokens,
                                                  GSList **result);
static gboolean gibbon_clip_parse_setting (const gchar *key,
                                           const gchar *value,
                                           GSList **result);
static gboolean gibbon_clip_parse_setting1 (const gchar *line,
                                            gchar **tokens,
                                            GSList **result);
static gboolean gibbon_clip_parse_setting2 (const gchar *line,
                                            gchar **tokens,
                                            GSList **result);
static gboolean gibbon_clip_parse_start_toggles (const gchar *line,
                                                 gchar **tokens,
                                                 GSList **result);
static gboolean gibbon_clip_parse_toggle (const gchar *key, gboolean value,
                                          GSList **result);
static gboolean gibbon_clip_parse_toggle1 (const gchar *line,
                                           gchar **tokens,
                                           GSList **result);
static gboolean gibbon_clip_parse_2stars (const gchar *line,
                                          gchar **tokens,
                                          GSList **result);
static gboolean gibbon_clip_parse_start_saved (const gchar *line,
                                               gchar **tokens,
                                               GSList **result);
static gboolean gibbon_clip_parse_show_saved (const gchar *line,
                                              gchar **tokens,
                                              GSList **result);
static gboolean gibbon_clip_parse_show_saved_none (const gchar *line,
                                                   gchar **tokens,
                                                   GSList **result);
static gboolean gibbon_clip_parse_saved_count (const gchar *line,
                                               gchar **tokens,
                                               GSList **result);
static gboolean gibbon_clip_parse_show_address (const gchar *line,
                                                gchar **tokens,
                                                GSList **result);

static gboolean gibbon_clip_parse_not_email_address (gchar *quoted,
                                                     GSList **result);
static gboolean gibbon_clip_parse_movement (gchar *string, GSList **result);

static GSList *gibbon_clip_alloc_int (GSList *list, enum GibbonClipType type,
                                      gint64 value);
static GSList *gibbon_clip_alloc_double (GSList *list, enum GibbonClipType type,
                                         gdouble value);
static GSList *gibbon_clip_alloc_string (GSList *list, enum GibbonClipType type,
                                         const gchar *value);
static gboolean gibbon_clip_extract_integer (const gchar *str, gint64 *result,
                                             gint64 lower, gint64 upper);
static gboolean gibbon_clip_extract_double (const gchar *str, gdouble *result,
                                            gdouble lower, gdouble upper);
static gboolean gibbon_clip_chomp (gchar *str, gchar c);

static gboolean gibbon_clip_toggle_ready (GSList **result, gboolean ready);

GSList *
gibbon_clip_parse (const gchar *line)
{
        gboolean success = FALSE;
        GSList *result = NULL;
        gchar **tokens = gibbon_strsplit_ws (line);
        gchar *first;

        if (!tokens || !tokens[0]) {
                g_strfreev (tokens);
                result = gibbon_clip_alloc_int (result, GIBBON_CLIP_TYPE_UINT,
                                                GIBBON_CLIP_CODE_EMPTY);
                return result;
        }

        first = tokens[0];

        if (first[0] >= '1' && first[0] <= '9' && !first[1])
                success = gibbon_clip_parse_clip (line, tokens, &result);
        else if (first[0] == '1' && first[1] >= '0' && first[1] <= '9'
                 && !first[2])
                success = gibbon_clip_parse_clip (line, tokens, &result);

        if (!success) {
                gibbon_clip_free_result (result);
                result = NULL;
                switch (first[0]) {
                case '*':
                        if ('*' == first[1] && !first[2])
                                success = gibbon_clip_parse_2stars (line,
                                                                    tokens,
                                                                    &result);
                        break;
                case 'a':
                        if (0 == g_strcmp0 ("allowpip", first)
                            || 0 == g_strcmp0 ("autoboard", first)
                            || 0 == g_strcmp0 ("autodouble", first)
                            || 0 == g_strcmp0 ("automove", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 'b':
                        if (0 == strncmp ("board:", first, 6))
                                success = gibbon_clip_parse_board (line, tokens,
                                                                   &result);
                        else if (0 == g_strcmp0 ("boardstyle:", first))
                                success = gibbon_clip_parse_setting1 (line,
                                                                      tokens,
                                                                      &result);
                        else if (0 == g_strcmp0 ("bell", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 'c':
                        if (0 == g_strcmp0 ("crawford", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                case 'd':
                        if (0 == g_strcmp0 ("double", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                case 'g':
                        if (0 == g_strcmp0 ("greedy", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                case 'l':
                        if (0 == g_strcmp0 ("linelength:", first))
                                success = gibbon_clip_parse_setting1 (line,
                                                                      tokens,
                                                                      &result);
                        break;
                case 'm':
                        if (0 == g_strcmp0 ("moreboards", first)
                            || 0 == g_strcmp0 ("moves", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 'n':
                        if (0 == g_strcmp0 ("notify", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        else if (0 == g_strcmp0 ("no", first)
                                 || 0 == g_strcmp0 ("saved", tokens[1])
                                 || 0 == g_strcmp0 ("games.", tokens[2]))
                                success = gibbon_clip_parse_show_saved_none (line,
                                                                         tokens,
                                                                       &result);
                        break;
                case 'o':
                        if (0 == g_strcmp0 ("opponent", first)
                            && 0 == g_strcmp0 ("matchlength", tokens[1])
                            && 0 == g_strcmp0 ("score", tokens[2])
                            && 0 == g_strcmp0 ("(user's", tokens[3])
                            && 0 == g_strcmp0 ("points", tokens[4])
                            && 0 == g_strcmp0 ("first)", tokens[5])
                            && !tokens[6])
                                success = gibbon_clip_parse_start_saved (line,
                                                                         tokens,
                                                                       &result);
                        break;
                case 'p':
                        if (0 == g_strcmp0 ("pagelength:", first))
                                success = gibbon_clip_parse_setting1 (line,
                                                                      tokens,
                                                                      &result);
                        break;
                case 'r':
                        if (0 == g_strcmp0 ("redoubles:", first))
                                success = gibbon_clip_parse_setting1 (line,
                                                                      tokens,
                                                                      &result);
                        else if (0 == g_strcmp0 ("ratings", first)
                                || 0 == g_strcmp0 ("ready", first)
                                || 0 == g_strcmp0 ("report", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 's':
                        if (0 == g_strcmp0 ("sortwho:", first))
                                success = gibbon_clip_parse_setting1 (line,
                                                                      tokens,
                                                                      &result);
                        else if (0 == g_strcmp0 ("silent", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 't':
                        if (0 == g_strcmp0 ("timezone:", first))
                                success = gibbon_clip_parse_setting1 (line,
                                                                      tokens,
                                                                      &result);
                        else if (0 == g_strcmp0 ("telnet", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 'w':
                        if (0 == g_strcmp0 ("wrap", first))
                                success = gibbon_clip_parse_toggle1 (line,
                                                                     tokens,
                                                                     &result);
                        break;
                case 'S':
                        if (0 == g_strcmp0 ("Settings", tokens[0])
                            && 0 == g_strcmp0 ("of", tokens[1])
                            && 0 == g_strcmp0 ("variables:", tokens[2])
                            && !tokens[3])
                                success =
                                        gibbon_clip_parse_start_settings (line,
                                                                       tokens,
                                                                       &result);
                        break;
                case 'T':
                        if (0 == g_strcmp0 ("The", tokens[0])
                            && 0 == g_strcmp0 ("current", tokens[1])
                            && 0 == g_strcmp0 ("settings", tokens[2])
                            && 0 == g_strcmp0 ("are:", tokens[3])
                            && !tokens[4])
                                success =
                                        gibbon_clip_parse_start_toggles (line,
                                                                       tokens,
                                                                       &result);
                        break;
                case 'V':
                        if (0 == g_strcmp0 ("Value", first)
                            && 0 == g_strcmp0 ("of", tokens[1])
                            && tokens[2]
                            && 0 == g_strcmp0 ("set", tokens[3])
                            && 0 == g_strcmp0 ("to", tokens[4])
                            && tokens[5] && !tokens[6])
                                success = gibbon_clip_parse_setting2 (line,
                                                                      tokens,
                                                                      &result);
                                break;
                case 'Y':
                        if (0 == g_strcmp0 ("You're", tokens[0])
                            && 0 == g_strcmp0 ("now", tokens[1])
                            && 0 == g_strcmp0 ("watching", tokens[2]))
                                success =
                                        gibbon_clip_parse_youre_watching (line,
                                                                         tokens,
                                                                       &result);
                        else if (0 == g_strcmp0 ("Your", tokens[0])
                                 && 0 == g_strcmp0 ("email", tokens[1])
                                 && 0 == g_strcmp0 ("address", tokens[2])
                                 && 0 == g_strcmp0 ("is", tokens[3])) {
                                success = gibbon_clip_parse_show_address (line,
                                                                         tokens,
                                                                       &result);
                        }
                        break;
                }
        }

        if (!success && tokens[1]) {
                gibbon_clip_free_result (result);
                result = NULL;

                switch (tokens[1][0]) {
                case 'a':
                        if (0 == g_strcmp0 ("and", tokens[1]))
                                success = gibbon_clip_parse_and (line, tokens,
                                                                 &result);
                        break;
                case 'h':
                        if (0 == g_strcmp0 ("has", tokens[1])
                            && 0 == g_strcmp0 ("saved", tokens[3])
                            && (0 == g_strcmp0 ("games.", tokens[4])
                                || 0 == g_strcmp0 ("game.", tokens[4]))
                            && !tokens[5])
                                success = gibbon_clip_parse_saved_count (line,
                                                                         tokens,
                                                                       &result);
                        break;
                case 'm':
                        if (0 == g_strcmp0 ("moves", tokens[1]))
                                success = gibbon_clip_parse_moves (line, tokens,
                                                                  &result);
                        break;
                case 'r':
                        if (0 == g_strcmp0 ("rolls", tokens[1]))
                                success = gibbon_clip_parse_rolls (line, tokens,
                                                                   &result);
                        break;
                case 'w':
                        if (0 == g_strcmp0 ("wins", tokens[1]))
                                success = gibbon_clip_parse_wins (line, tokens,
                                                                  &result);
                        else if (0 == g_strcmp0 ("wants", tokens[1]))
                                success = gibbon_clip_parse_wants (line, tokens,
                                                                   &result);
                        break;
                }
        }

        if (!success) {
                gibbon_clip_free_result (result);
                result = NULL;

                switch (g_strv_length (tokens)) {
                        case 5:
                                success = gibbon_clip_parse_show_saved (line,
                                                                        tokens,
                                                                        &result);
                                break;
                }
        }

        g_strfreev (tokens);
        if (!success) {
                gibbon_clip_free_result (result);
                return NULL;
        }

        return g_slist_reverse (result);
}

static gboolean
gibbon_clip_parse_clip (const gchar *line, gchar **tokens,
                        GSList **result)
{
        enum GibbonClipCode code;
        gchar *first = tokens[0];

        code = first[0] - '0';
        if (first[1]) {
                code *= 10;
                code += first[1] - '0';
        }

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                               (gint64) code);

        switch (code) {
                case GIBBON_CLIP_CODE_WELCOME:
                        return gibbon_clip_parse_clip_welcome (line, tokens,
                                                               result);
                case GIBBON_CLIP_CODE_OWN_INFO:
                        return gibbon_clip_parse_clip_own_info (line, tokens,
                                                                result);
                case GIBBON_CLIP_CODE_WHO_INFO:
                        return gibbon_clip_parse_clip_who_info (line, tokens,
                                                                result);
                case GIBBON_CLIP_CODE_LOGIN:
                case GIBBON_CLIP_CODE_LOGOUT:
                        return gibbon_clip_parse_clip_somebody_message (line,
                                                                        tokens,
                                                                        result);
                case GIBBON_CLIP_CODE_SAYS:
                case GIBBON_CLIP_CODE_SHOUTS:
                case GIBBON_CLIP_CODE_KIBITZES:
                case GIBBON_CLIP_CODE_WHISPERS:
                case GIBBON_CLIP_CODE_YOU_SAY:
                        return gibbon_clip_parse_clip_somebody_something (line,
                                                                          tokens,
                                                                          result);
                case GIBBON_CLIP_CODE_MESSAGE:
                        return gibbon_clip_parse_clip_message (line, tokens,
                                                               result);
                case GIBBON_CLIP_CODE_MESSAGE_DELIVERED:
                case GIBBON_CLIP_CODE_MESSAGE_SAVED:
                        return gibbon_clip_parse_clip_somebody (line,
                                                                tokens,
                                                                result);
                case GIBBON_CLIP_CODE_YOU_SHOUT:
                case GIBBON_CLIP_CODE_YOU_KIBITZ:
                case GIBBON_CLIP_CODE_YOU_WHISPER:
                        return gibbon_clip_parse_clip_something (line, tokens,
                                                                 result);
                case GIBBON_CLIP_CODE_MOTD:
                case GIBBON_CLIP_CODE_MOTD_END:
                case GIBBON_CLIP_CODE_WHO_INFO_END:
                        if (1 == g_strv_length (tokens))
                                return TRUE;
                default:
                        break;
        }

        return FALSE;
}

static gboolean
gibbon_clip_parse_clip_welcome (const gchar *line, gchar **tokens,
                                GSList **result)
{
        gint64 timestamp;

        if (4 != g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);
        if (!gibbon_clip_extract_integer (tokens[2], &timestamp,
                                          G_MININT, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_TIMESTAMP,
                                         timestamp);

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            tokens[3]);

        return TRUE;
}


static gboolean
gibbon_clip_parse_clip_own_info (const gchar *line, gchar **tokens,
                                 GSList **result)
{
        gint64 i;
        gdouble d;

        if (22 != g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);

        if (!gibbon_clip_extract_integer (tokens[2], &i,
                                          0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[3], &i,
                                          0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[4], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[5], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[6], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[7], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[8], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[9], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[10], &i, 0, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i);

        if (!gibbon_clip_extract_integer (tokens[11], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[12], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[13], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[14], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_double (tokens[15], &d, 0, G_MAXDOUBLE))
                return FALSE;
        *result = gibbon_clip_alloc_double (*result, GIBBON_CLIP_TYPE_DOUBLE,
                                            d);

        if (!gibbon_clip_extract_integer (tokens[16], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[17], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (g_strcmp0 ("unlimited", tokens[18])) {
                if (!gibbon_clip_extract_integer (tokens[18], &i,
                                                  0, G_MAXINT))
                        return FALSE;
        } else {
                i = -1;
        }
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[19], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[20], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            tokens[21]);

        return TRUE;
}

static gboolean
gibbon_clip_parse_clip_who_info (const gchar *line, gchar **tokens,
                                 GSList **result)
{
        gint64 i;
        gdouble d;
        gchar *s;

        if (13 != g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);

        s = tokens[2];
        if ('-' == s[0] && !s[1])
                s = "";
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME, s);

        s = tokens[3];
        if ('-' == s[0] && !s[1])
                s = "";
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME, s);

        if (!gibbon_clip_extract_integer (tokens[4], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_integer (tokens[5], &i, 0, 1))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN, i);

        if (!gibbon_clip_extract_double (tokens[6], &d, 0, G_MAXDOUBLE))
                return FALSE;
        *result = gibbon_clip_alloc_double (*result, GIBBON_CLIP_TYPE_DOUBLE,
                                            d);

        if (!gibbon_clip_extract_integer (tokens[7], &i, 0, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i);

        if (!gibbon_clip_extract_integer (tokens[8], &i, 0, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i);

        if (!gibbon_clip_extract_integer (tokens[9], &i, 0, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_TIMESTAMP,
                                         i);

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            tokens[10]);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            tokens[11]);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            tokens[12]);

        return TRUE;
}

static gboolean
gibbon_clip_parse_clip_somebody_message (const gchar *line, gchar **tokens,
                                         GSList **result)
{
        const gchar *s;
        gsize length;
        gchar *s2;

        if (3 > g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);

        s = gibbon_skip_ws_tokens (line, (const gchar * const *) tokens, 2);
        length = 1 + strlen (s);
        s2 = g_alloca (length);
        memcpy (s2, s, length);

        while (gibbon_clip_chomp (s2, '.')) {}
        while (gibbon_clip_chomp (s2, ' ')) {}
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            s2);

        return TRUE;
}


static gboolean
gibbon_clip_parse_clip_somebody_something (const gchar *line, gchar **tokens,
                                           GSList **result)
{
        const gchar *s;

        if (3 > g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);

        s = gibbon_skip_ws_tokens (line, (const gchar * const *) tokens, 2);

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            s);

        return TRUE;
}

static gboolean
gibbon_clip_parse_clip_message (const gchar *line, gchar **tokens,
                                GSList **result)
{
        gint64 i;
        const gchar *s;

        if (4 >= g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);

        if (!gibbon_clip_extract_integer (tokens[2], &i, G_MININT, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_TIMESTAMP,
                                         i);

        s = gibbon_skip_ws_tokens (line, (const gchar * const* const) tokens,
                                   3);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            s);

        return TRUE;
}

static gboolean
gibbon_clip_parse_clip_somebody (const gchar *line, gchar **tokens,
                                 GSList **result)
{
        if (2 != g_strv_length (tokens))
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);

        return TRUE;
}

static gboolean
gibbon_clip_parse_clip_something (const gchar *line, gchar **tokens,
                                  GSList **result)
{
        const gchar *s;

        if (2 > g_strv_length (tokens))
                return FALSE;

        s = gibbon_skip_ws_tokens (line, (const gchar * const *) tokens, 1);

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            s);

        return TRUE;
}

static gboolean
gibbon_clip_parse_board (const gchar *line, gchar **_tokens,
                         GSList **result)
{
        gchar **tokens;
        gsize num_tokens;
        gboolean retval = FALSE;
        gint64 color, direction, turn;
        gint64 i64;
        gint i;
        gboolean is_bad_board = FALSE;
        gchar *board;
        gchar *fifty_third;
        gchar c;

        tokens = g_strsplit (line, ":", 53);
        num_tokens = g_strv_length (tokens);

#ifdef GIBBON_CLIP_DEBUG_BOARD_STATE
        gibbon_clip_dump_board (line, tokens);
#endif

        if (num_tokens < 53)
                goto bail_out_board;

        fifty_third = tokens[52];
        if (fifty_third[0] == '0'
            && (fifty_third[1] >= '0' && fifty_third[1] <= '9')) {
                is_bad_board = TRUE;
        } else {
                while (*fifty_third) {
                        if (*fifty_third < '0' || *fifty_third > '9') {
                                is_bad_board = TRUE;
                                break;
                        }
                        ++fifty_third;
                }
        }

        if (is_bad_board) {
                *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                                 GIBBON_CLIP_CODE_BAD_BOARD);
                c = tokens[52][1];
                tokens[52][1] = 0;

                board = g_strjoinv (":", tokens);
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    board);
                g_free (board);

                tokens[52][1] = c;
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    tokens[52] + 1);

                g_strfreev (tokens);

                return TRUE;
        }

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_BOARD);

        /* Player's name.  */
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[1]);
        /* Opponent's name.  */
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[2]);

        /* Match length.  */
        if (!gibbon_clip_extract_integer (tokens[3], &i64, 0, G_MAXINT))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i64);

        /* Scores.  */
        if (!gibbon_clip_extract_integer (tokens[4], &i64, 0, G_MAXINT))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i64);
        if (!gibbon_clip_extract_integer (tokens[5], &i64, 0, G_MAXINT))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i64);

        /* Color.  */
        if (!gibbon_clip_extract_integer (tokens[41], &color, -1, 1))
                goto bail_out_board;

        /* Playing direction.  */
        if (!gibbon_clip_extract_integer (tokens[42], &direction, -1, 1))
                goto bail_out_board;

        if (!direction)
                goto bail_out_board;

        /* Regular points.  */
        if (direction == GIBBON_POSITION_SIDE_BLACK) {
                for (i = 6; i < 30; ++i) {
                        if (!gibbon_clip_extract_integer (tokens[i + 1], &i64,
                                                          -15, 15))
                                goto bail_out_board;

                        i64 *= color;

                        *result = gibbon_clip_alloc_int (*result,
                                                         GIBBON_CLIP_TYPE_INT,
                                                         i64);
                }
        } else {
                for (i = 29; i >= 6; --i) {
                        if (!gibbon_clip_extract_integer (tokens[i + 1], &i64,
                                                          -15, 15))
                                goto bail_out_board;

                        i64 *= color;

                        *result = gibbon_clip_alloc_int (*result,
                                                         GIBBON_CLIP_TYPE_INT,
                                                         i64);
                }
        }

        if (!gibbon_clip_extract_integer (tokens[32], &turn, -1, 1))
                goto bail_out_board;

        /* Dice.  */
        if (turn == color) {
                if (!gibbon_clip_extract_integer (tokens[33], &i64,
                                                  0, 6))
                        goto bail_out_board;
                *result = gibbon_clip_alloc_int (*result,
                                                 GIBBON_CLIP_TYPE_INT,
                                                 i64);
                if (!gibbon_clip_extract_integer (tokens[34], &i64,
                                                  0, 6))
                        goto bail_out_board;
                *result = gibbon_clip_alloc_int (*result,
                                                 GIBBON_CLIP_TYPE_INT,
                                                 i64);
        } else if (turn) {
                if (!gibbon_clip_extract_integer (tokens[35], &i64,
                                                  0, 6))
                        goto bail_out_board;
                *result = gibbon_clip_alloc_int (*result,
                                                 GIBBON_CLIP_TYPE_INT,
                                                 -i64);
                if (!gibbon_clip_extract_integer (tokens[36], &i64,
                                                  0, 6))
                        goto bail_out_board;
                *result = gibbon_clip_alloc_int (*result,
                                                 GIBBON_CLIP_TYPE_INT,
                                                 -i64);
        }

        /* Cube.  */
        if (!gibbon_clip_extract_integer (tokens[37], &i64,
                                          1, G_MAXINT))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result,
                                         GIBBON_CLIP_TYPE_UINT,
                                         i64);

        /* May double?  */
        if (!gibbon_clip_extract_integer (tokens[38], &i64,
                                          0, 1))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result,
                                         GIBBON_CLIP_TYPE_BOOLEAN,
                                         i64);
        if (!gibbon_clip_extract_integer (tokens[39], &i64,
                                          0, 1))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result,
                                         GIBBON_CLIP_TYPE_BOOLEAN,
                                         i64);

        /* Checkers on bar.  */
        if (!gibbon_clip_extract_integer (tokens[47], &i64,
                                          0, 15))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result,
                                         GIBBON_CLIP_TYPE_UINT,
                                         i64);
        if (!gibbon_clip_extract_integer (tokens[48], &i64,
                                          0, 15))
                goto bail_out_board;
        *result = gibbon_clip_alloc_int (*result,
                                         GIBBON_CLIP_TYPE_UINT,
                                         i64);

        retval = TRUE;

bail_out_board:
        g_strfreev (tokens);

        return retval;
}

static gboolean
gibbon_clip_parse_youre_watching (const gchar *line, gchar **tokens,
                                  GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_YOURE_WATCHING);
        gibbon_clip_chomp (tokens[3], '.');
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[3]);

        return TRUE;
}

static gboolean
gibbon_clip_parse_and (const gchar *line, gchar **tokens, GSList **result)
{
        if (!tokens[2])
                return FALSE;
        if (!tokens[3])
                return FALSE;

        if (0 == g_strcmp0 ("are", tokens[3])) {
                if (0 == g_strcmp0 ("resuming", tokens[4])
                    && 0 == g_strcmp0 ("their", tokens[5]))
                        return gibbon_clip_parse_resuming (line, tokens,
                                                           result);
        } else if (0 == g_strcmp0 ("start", tokens[3])) {
                return gibbon_clip_parse_start_match (line, tokens, result);
        }

        return TRUE;
}


static gboolean
gibbon_clip_parse_resuming (const gchar *line, gchar **tokens,
                            GSList **result)
{
        gint64 length;
        gchar *ptr;

        if (8 != g_strv_length (tokens))
                return FALSE;

        if (g_strcmp0 ("match.", tokens[7]))
                return FALSE;

        if (0 == g_strcmp0 ("unlimited", tokens[6])) {
                length = 0;
        } else {
#ifndef HAVE_INDEX
# define index(str, c) memchr (str, c, strlen (str))
#endif
                ptr = index (tokens[6], '-');
                if (!ptr)
                        return FALSE;
                if (g_strcmp0 ("-point", ptr))
                        return FALSE;
                *ptr = 0;
                if (!gibbon_clip_extract_integer (tokens[6], &length,
                                                  1, G_MAXINT))
                        return FALSE;
        }

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_RESUME_MATCH);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[0]);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[2]);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         length);

        return TRUE;
}

static gboolean
gibbon_clip_parse_start_match (const gchar *line, gchar **tokens,
                               GSList **result)
{
        gint64 length = -1;

        if ((0 == g_strcmp0 ("a", tokens[4])
             || 0 == g_strcmp0 ("an", tokens[4]))) {
                if (0 == g_strcmp0 ("unlimited", tokens[5])
                    && 0 == g_strcmp0 ("match.", tokens[6])
                    && !tokens[7]) {
                        length = 0;
                } else if (gibbon_clip_extract_integer (tokens[5], &length,
                                                        1, G_MAXINT)
                           && 0 == g_strcmp0 ("point", tokens[6])
                           && 0 == g_strcmp0 ("match", tokens[7])
                           && !tokens[8]) {
                        /* Side-effect from extracting integer.  */
                }
        }

        if  (length == -1)
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_START_MATCH);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[0]);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[2]);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         length);

        return TRUE;
}

static gboolean
gibbon_clip_parse_wins (const gchar *line, gchar **tokens, GSList **result)
{
        gint64 i64;
        gint64 s0, s1;
        gchar *ptr;

        if (10 != g_strv_length (tokens))
                return FALSE;

        if (g_strcmp0 ("a", tokens[2]))
                return FALSE;
        if (g_strcmp0 ("point", tokens[4]))
                return FALSE;
        if (g_strcmp0 ("match", tokens[5]))
                return FALSE;
        if (g_strcmp0 ("against", tokens[6]))
                return FALSE;
        if (g_strcmp0 (".", tokens[9]))
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_WIN_MATCH);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[0]);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[7]);

        if (!gibbon_clip_extract_integer (tokens[3], &i64, 1, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i64);

        ptr = index (tokens[8], '-');
        if (!ptr)
                return FALSE;

        *ptr++ = 0;
        if (!gibbon_clip_extract_integer (tokens[8], &s0, 0, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, s0);
        if (!gibbon_clip_extract_integer (ptr, &s1, 0, G_MAXINT))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, s1);

        if (s0 < i64)
                return FALSE;
        if (s0 < s1)
                return FALSE;
        if (s1 >= i64)
                return FALSE;

        return TRUE;
}

static gboolean
gibbon_clip_parse_wants (const gchar *line, gchar **tokens, GSList **result)
{
        guint num_tokens = g_strv_length (tokens);
        gint64 length;

        switch (num_tokens) {
        case 9:
                if (0 == g_strcmp0 ("wants", tokens[1])
                    && 0 == g_strcmp0 ("to", tokens[2])
                    && 0 == g_strcmp0 ("play", tokens[3])
                    && 0 == g_strcmp0 ("an", tokens[4])
                    && 0 == g_strcmp0 ("unlimited", tokens[5])
                    && 0 == g_strcmp0 ("match", tokens[6])
                    && 0 == g_strcmp0 ("with", tokens[7])
                    && 0 == g_strcmp0 ("you.", tokens[8])) {
                        *result = gibbon_clip_alloc_int (*result,
                                                         GIBBON_CLIP_TYPE_UINT,
                                                         GIBBON_CLIP_CODE_INVITATION);
                        *result = gibbon_clip_alloc_string (*result,
                                                          GIBBON_CLIP_TYPE_NAME,
                                                            tokens[0]);
                        *result = gibbon_clip_alloc_int (*result,
                                                         GIBBON_CLIP_TYPE_UINT,
                                                         0);
                        return TRUE;
                }
                break;
        case 10:
                if (0 == g_strcmp0 ("wants", tokens[1])
                    && 0 == g_strcmp0 ("to", tokens[2])
                    && 0 == g_strcmp0 ("play", tokens[3])
                    /* This is "a" not "an" even for 8 and 11! */
                    && 0 == g_strcmp0 ("a", tokens[4])
                    && 0 == g_strcmp0 ("point", tokens[6])
                    && 0 == g_strcmp0 ("match", tokens[7])
                    && 0 == g_strcmp0 ("with", tokens[8])
                    && 0 == g_strcmp0 ("you.", tokens[9])) {
                        if (!gibbon_clip_extract_integer (tokens[5], &length,
                                                          0, G_MAXINT))
                                return FALSE;
                        *result = gibbon_clip_alloc_int (*result,
                                                         GIBBON_CLIP_TYPE_UINT,
                                                   GIBBON_CLIP_CODE_INVITATION);
                        *result = gibbon_clip_alloc_string (*result,
                                                          GIBBON_CLIP_TYPE_NAME,
                                                            tokens[0]);
                        *result = gibbon_clip_alloc_int (*result,
                                                         GIBBON_CLIP_TYPE_UINT,
                                                         length);
                        return TRUE;
                }
                break;
        }

        return FALSE;
}

static gboolean
gibbon_clip_parse_rolls (const gchar *line, gchar **tokens, GSList **result)
{
        gint64 i64;

        if (5 != g_strv_length (tokens))
                return FALSE;

        if (g_strcmp0 ("and", tokens[3]))
                return FALSE;

        gibbon_clip_chomp (tokens[4], '.');

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_ROLLS);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[0]);

        if (!gibbon_clip_extract_integer (tokens[2], &i64, 1, 6))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i64);
        if (!gibbon_clip_extract_integer (tokens[4], &i64, 1, 6))
                return FALSE;
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, i64);

        return TRUE;
}

static gboolean
gibbon_clip_parse_moves (const gchar *line, gchar **tokens, GSList **result)
{
        gint64 num_movements = g_strv_length (tokens) - 3;
        gint i;

        if (num_movements < 1 || num_movements > 4)
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_MOVES);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[0]);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         num_movements);
        for (i = 0; i < num_movements; ++i)
                if (!gibbon_clip_parse_movement (tokens[i + 2], result))
                        return FALSE;

        return TRUE;
}

static gboolean
gibbon_clip_parse_start_settings (const gchar *line, gchar **tokens,
                                  GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_START_SETTINGS);

        return TRUE;
}

static gboolean
gibbon_clip_parse_setting (const gchar *key, const gchar *value,
                           GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_SHOW_SETTING);

        if (0 == g_strcmp0 ("boardstyle", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "boardstyle");
        else if (0 == g_strcmp0 ("linelength", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "linelength");
        else if (0 == g_strcmp0 ("pagelength", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "pagelength");
        else if (0 == g_strcmp0 ("redoubles", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "redoubles");
        else if (0 == g_strcmp0 ("sortwho", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "sortwho");
        else if (0 == g_strcmp0 ("timezone", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "timezone");
        else
                return FALSE;

        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            value);

        return TRUE;
}

static gboolean
gibbon_clip_parse_setting1 (const gchar *line, gchar **tokens,
                            GSList **result)
{
        gchar *key;

        if (!tokens[1] || tokens[2])
                return FALSE;

        key = g_alloca (1 + strlen (tokens[0]));
        strcpy (key, tokens[0]);
        gibbon_clip_chomp (key, ':');

        return gibbon_clip_parse_setting (key, tokens[1], result);
}

static gboolean
gibbon_clip_parse_setting2 (const gchar *line, gchar **tokens,
                            GSList **result)
{
        gchar *key;
        gchar *value;

        key = g_alloca (1 + strlen (tokens[2]));
        strcpy (key, tokens[2]);
        ++key;
        gibbon_clip_chomp (key, '\'');

        value = g_alloca (1 + strlen (tokens[5]));
        strcpy (value, tokens[5]);
        gibbon_clip_chomp (value, '.');

        return gibbon_clip_parse_setting (key, value, result);
}

static gboolean
gibbon_clip_parse_start_toggles (const gchar *line, gchar **tokens,
                                  GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_START_TOGGLES);

        return TRUE;
}

static gboolean
gibbon_clip_parse_toggle (const gchar *key, gboolean value, GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_SHOW_TOGGLE);

        if (0 == g_strcmp0 ("allowpip", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "allowpip");
        else if (0 == g_strcmp0 ("autoboard", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "autoboard");
        else if (0 == g_strcmp0 ("autodouble", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "autodouble");
        else if (0 == g_strcmp0 ("automove", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "automove");
        else if (0 == g_strcmp0 ("bell", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "bell");
        else if (0 == g_strcmp0 ("crawford", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "crawford");
        else if (0 == g_strcmp0 ("double", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "double");
        else if (0 == g_strcmp0 ("greedy", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "greedy");
        else if (0 == g_strcmp0 ("moreboards", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "moreboards");
        else if (0 == g_strcmp0 ("moves", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "moves");
        else if (0 == g_strcmp0 ("notify", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "notify");
        else if (0 == g_strcmp0 ("ratings", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "ratings");
        else if (0 == g_strcmp0 ("ready", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "ready");
        else if (0 == g_strcmp0 ("report", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "report");
        else if (0 == g_strcmp0 ("silent", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "silent");
        else if (0 == g_strcmp0 ("telnet", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "telnet");
        else if (0 == g_strcmp0 ("wrap", key))
                *result = gibbon_clip_alloc_string (*result,
                                                    GIBBON_CLIP_TYPE_STRING,
                                                    "wrap");
        else
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN,
                                         value);

        return TRUE;
}

static gboolean
gibbon_clip_parse_toggle1 (const gchar *line, gchar **tokens,
                           GSList **result)
{
        gboolean value;

        if (!tokens[1] || tokens[2])
                return FALSE;

        if (0 == g_strcmp0 ("YES", tokens[1]))
                value = TRUE;
        else if (0 == g_strcmp0 ("NO", tokens[1]))
                value = FALSE;
        else
                return FALSE;

        return gibbon_clip_parse_toggle (tokens[0], value, result);
}


static gboolean
gibbon_clip_parse_2stars (const gchar *line, gchar **tokens,
                           GSList **result)
{
        if (0 == g_strcmp0 ("You", tokens[1])) {
                if (0 == g_strcmp0 ("won't", tokens[2])
                    && 0 == g_strcmp0 ("be", tokens[3])
                    && 0 == g_strcmp0 ("notified", tokens[4])
                    && 0 == g_strcmp0 ("when", tokens[5])
                    && 0 == g_strcmp0 ("new", tokens[6])
                    && 0 == g_strcmp0 ("users", tokens[7])
                    && 0 == g_strcmp0 ("log", tokens[8])
                    && 0 == g_strcmp0 ("in.", tokens[9]))
                    return gibbon_clip_parse_toggle ("notify", FALSE, result);
        }

        if (0 == g_strcmp0 ("You'll", tokens[1])) {
                if (0 == g_strcmp0 ("be", tokens[2])
                    && 0 == g_strcmp0 ("notified", tokens[3])
                    && 0 == g_strcmp0 ("when", tokens[4])
                    && 0 == g_strcmp0 ("new", tokens[5])
                    && 0 == g_strcmp0 ("users", tokens[6])
                    && 0 == g_strcmp0 ("log", tokens[7])
                    && 0 == g_strcmp0 ("in.", tokens[8]))
                    return gibbon_clip_parse_toggle ("notify", TRUE, result);
        } else if (0 == g_strcmp0 ("You're", tokens[1])) {
                if (0 == g_strcmp0 ("now", tokens[2])
                    && 0 == g_strcmp0 ("ready", tokens[3])
                    && 0 == g_strcmp0 ("to", tokens[4])
                    && 0 == g_strcmp0 ("invite", tokens[5])
                    && 0 == g_strcmp0 ("or", tokens[6])
                    && 0 == g_strcmp0 ("join", tokens[7])
                    && 0 == g_strcmp0 ("someone.", tokens[8])) {
                        return gibbon_clip_toggle_ready (result, TRUE);
                }
                if (0 == g_strcmp0 ("now", tokens[2])
                    && 0 == g_strcmp0 ("refusing", tokens[3])
                    && 0 == g_strcmp0 ("to", tokens[4])
                    && 0 == g_strcmp0 ("play", tokens[5])
                    && 0 == g_strcmp0 ("with", tokens[6])
                    && 0 == g_strcmp0 ("someone.", tokens[7])) {
                        return gibbon_clip_toggle_ready (result, FALSE);
                }
        }

        if (0 == g_strcmp0 ("is", tokens[2])) {
                if (0 == g_strcmp0 ("not", tokens[3])
                    && 0 == g_strcmp0 ("an", tokens[4])
                    && 0 == g_strcmp0 ("email", tokens[5])
                    && 0 == g_strcmp0 ("address.", tokens[6])
                    && !tokens[7])
                    return gibbon_clip_parse_not_email_address (tokens[1],
                                                                result);
        }

        return FALSE;
}

static gboolean
gibbon_clip_parse_start_saved (const gchar *line, gchar **tokens,
                               GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_SHOW_START_SAVED);

        return TRUE;
}

static gboolean
gibbon_clip_parse_show_saved_none (const gchar *line, gchar **tokens,
                                   GSList **result)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_SHOW_SAVED_NONE);

        return TRUE;
}

static gboolean
gibbon_clip_parse_show_saved (const gchar *line, gchar **tokens,
                              GSList **result)
{
        gint64 match_length;
        gint64 user_score;
        gint64 other_score;
        gchar *opponent;

        if (tokens[3][0] != '-' || tokens[3][1])
                return FALSE;

        if (0 == g_strcmp0 ("unlimited", tokens[1]))
                match_length = 0;
        else if (!gibbon_clip_extract_integer (tokens[1], &match_length,
                                          1, G_MAXUINT))
                return FALSE;

        if (!gibbon_clip_extract_integer (tokens[2], &user_score,
                                          0, G_MAXUINT))
                return FALSE;
        if (!gibbon_clip_extract_integer (tokens[4], &other_score,
                                          0, G_MAXUINT))
                return FALSE;
        if (match_length) {
                if (user_score >= match_length)
                        return FALSE;
                if (other_score >= match_length)
                        return FALSE;
        }

        opponent = tokens[0];
        if ('*' == opponent[0])
                ++opponent;
        if ('*' == opponent[0])
                ++opponent;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_SHOW_SAVED);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            opponent);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         match_length);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         user_score);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         other_score);

        return TRUE;
}

static gboolean
gibbon_clip_parse_saved_count (const gchar *line, gchar **tokens,
                               GSList **result)
{
        gint64 num;

        if (0 == g_strcmp0 ("no", tokens[2]))
                num = 0;
        else if (!gibbon_clip_extract_integer (tokens[2], &num,
                                          1, G_MAXUINT))
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                          GIBBON_CLIP_CODE_SHOW_SAVED_COUNT);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_NAME,
                                            tokens[0]);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         num);

        return TRUE;
}

static gboolean
gibbon_clip_parse_show_address (const gchar *line, gchar **tokens,
                                GSList **result)
{
        gchar *quoted_address = tokens[4];
        gsize length;

        if (!quoted_address)
                return FALSE;

        gibbon_clip_chomp (quoted_address, '.');

        if ('\'' != quoted_address[0])
                return FALSE;

        length = strlen (quoted_address);

        if ('\'' != quoted_address[length - 1])
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                          GIBBON_CLIP_CODE_SHOW_ADDRESS);

        gibbon_clip_chomp (quoted_address, '\'');
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            quoted_address + 1);

        return TRUE;
}

static gboolean
gibbon_clip_parse_not_email_address (gchar *quoted_address,
                                     GSList **result)
{
        gsize length;

        if (!quoted_address)
                return FALSE;

        gibbon_clip_chomp (quoted_address, '.');

        if ('\'' != quoted_address[0])
                return FALSE;

        length = strlen (quoted_address);

        if ('\'' != quoted_address[length - 1])
                return FALSE;

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                       GIBBON_CLIP_CODE_ERROR_NO_EMAIL_ADDRESS);

        gibbon_clip_chomp (quoted_address, '\'');
        *result = gibbon_clip_alloc_string (*result,
                                            GIBBON_CLIP_TYPE_STRING,
                                            quoted_address + 1);

        return TRUE;
}

static gboolean
gibbon_clip_parse_movement (gchar *str, GSList **result)
{
        gint64 from, to;
        gchar *ptr;

        ptr = index (str, '-');
        if (!ptr)
                return FALSE;

        *ptr++ = 0;

        if (0 == g_strcmp0 ("bar", str))
                from = 0;
        else if (!gibbon_clip_extract_integer (str, &from, 1, 24))
                return FALSE;

        if (0 == g_strcmp0 ("off", ptr))
                to = 0;
        else if (!gibbon_clip_extract_integer (ptr, &to, 1, 24))
                return FALSE;

        if (!(from || to)) {
                return FALSE;
        } else if (!from) {
                if (to >= 19)
                        from = 25;
                else if (to > 6)
                        return FALSE;
        } else if (!to) {
                if (from >= 19)
                        to = 25;
                else if (from > 6)
                        return FALSE;
        }

        if (from == to) {
                return FALSE;
        } else if (from < to) {
                if (to - from > 6)
                        return FALSE;
        } else {
                if (from - to > 6)
                        return FALSE;
        }

        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, from);
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT, to);

        return TRUE;
}

static GSList *
gibbon_clip_alloc_int (GSList *list, enum GibbonClipType type, gint64 value)
{
        struct GibbonClipTokenSet *token = g_malloc (sizeof *token);

        token->type = type;
        token->v.i64 = value;

        return g_slist_prepend (list, token);
}

static GSList *
gibbon_clip_alloc_double (GSList *list, enum GibbonClipType type, gdouble value)
{
        struct GibbonClipTokenSet *token = g_malloc (sizeof *token);

        token->type = type;
        token->v.d = value;

        return g_slist_prepend (list, token);
}

static GSList *
gibbon_clip_alloc_string (GSList *list, enum GibbonClipType type,
                          const gchar *value)
{
        struct GibbonClipTokenSet *token = g_malloc (sizeof *token);

        token->type = type;
        token->v.s = g_strdup (value);

        return g_slist_prepend (list, token);
}

void
gibbon_clip_free_result (GSList *result)
{
        GSList *iter;
        struct GibbonClipTokenSet *set;

        if (G_UNLIKELY (!result))
                return;

        iter = result;
        while (iter) {
                set = (struct GibbonClipTokenSet *) iter->data;
                switch (set->type) {
                        case GIBBON_CLIP_TYPE_NAME:
                        case GIBBON_CLIP_TYPE_STRING:
                                g_free (set->v.s);
                                break;
                        default:
                                break;
                }
                iter = iter->next;
        }

        g_slist_free (result);
}

static gboolean
gibbon_clip_extract_integer (const gchar *str, gint64 *result,
                             gint64 lower, gint64 upper)
{
        char *endptr;
        gint64 r;

        if (!str)
                return FALSE;

        errno = 0;

        r = g_ascii_strtoll (str, &endptr, 10);

        if (errno)
                return FALSE;

        if (*endptr != 0)
                return FALSE;

        *result = (gint) r;
        if (*result < lower || *result > upper)
                return FALSE;

        return TRUE;
}

static gboolean
gibbon_clip_extract_double (const gchar *str, gdouble *result,
                            gdouble lower, gdouble upper)
{
        char *endptr;
        gdouble r;

        if (!str)
                return FALSE;

        errno = 0;

        r = g_ascii_strtod (str, &endptr);

        if (errno)
                return FALSE;

        if (*endptr != 0)
                return FALSE;

        *result = r;
        if (*result < lower || *result > upper)
                return FALSE;

        return TRUE;
}

static gboolean
gibbon_clip_chomp (gchar *str, gchar c)
{
        gsize length = strlen (str);

        if (length && str[length - 1] == c) {
                str[length - 1] = 0;
                return TRUE;
        }

        return FALSE;
}


#ifdef GIBBON_CLIP_DEBUG_BOARD_STATE
static void
gibbon_clip_dump_board (const gchar *raw,
                        gchar **tokens)
{
        int i;

        g_printerr ("=== Board ===\n");
        g_printerr ("board:%s\n", raw);
        for (i = 0; keys[i]; ++i)
                g_printerr ("%s (%s)\n", keys[i], tokens[i + 1]);
}
#endif

gboolean
gibbon_clip_get_uint64 (GSList **list, enum GibbonClipType type,
                        guint64 *value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list != NULL, FALSE);
        g_return_val_if_fail (*list != NULL, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = token->v.i64;

        *list = (*list)->next;

        return TRUE;
}

gboolean
gibbon_clip_get_int64 (GSList **list, enum GibbonClipType type,
                        gint64 *value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list, FALSE);
        g_return_val_if_fail (*list, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = (gint64) token->v.i64;

        *list = (*list)->next;

        return TRUE;
}

gboolean
gibbon_clip_get_uint (GSList **list, enum GibbonClipType type,
                      guint *value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list, FALSE);
        g_return_val_if_fail (*list, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = (guint) token->v.i64;

        *list = (*list)->next;

        return TRUE;
}

gboolean
gibbon_clip_get_int (GSList **list, enum GibbonClipType type,
                     gint *value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list, FALSE);
        g_return_val_if_fail (*list, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = (gint) token->v.i64;

        *list = (*list)->next;

        return TRUE;
}

gboolean
gibbon_clip_get_string (GSList **list, enum GibbonClipType type,
                        const gchar **value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list, FALSE);
        g_return_val_if_fail (*list, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = token->v.s;

        *list = (*list)->next;

        return TRUE;
}

gboolean
gibbon_clip_get_boolean (GSList **list, enum GibbonClipType type,
                         gboolean *value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list, FALSE);
        g_return_val_if_fail (*list, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = (gboolean) token->v.i64;

        *list = (*list)->next;

        return TRUE;
}

gboolean
gibbon_clip_get_double (GSList **list, enum GibbonClipType type,
                        gdouble *value)
{
        GSList *iter;
        struct GibbonClipTokenSet *token;

        g_return_val_if_fail (list, FALSE);
        g_return_val_if_fail (*list, FALSE);

        iter = *list;

        token = (struct GibbonClipTokenSet *) iter->data;

        if (token->type != type)
                return FALSE;

        *value = token->v.d;

        *list = (*list)->next;

        return TRUE;
}

static gboolean
gibbon_clip_toggle_ready (GSList **result, gboolean ready)
{
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_UINT,
                                         GIBBON_CLIP_CODE_SHOW_TOGGLE);
        *result = gibbon_clip_alloc_string (*result, GIBBON_CLIP_TYPE_STRING,
                                            "ready");
        *result = gibbon_clip_alloc_int (*result, GIBBON_CLIP_TYPE_BOOLEAN,
                                         ready);

        return TRUE;
}

