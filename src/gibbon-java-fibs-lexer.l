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

/*
 * Lexical scanner for the internal format of JavaFIBS.  The scanner is
 * quite forgiving.  For example, empty lines are ignored, as well as
 * whitespace between tokens.
 */

%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static int yywrap ();

#include <glib.h>
#include <glib/gi18n.h>

#define yylval gibbon_java_fibs_parser_lval

#include "gibbon-java-fibs-parser.h"
#include "gibbon-java-fibs-reader-priv.h"

#define alloc_name(s) \
	_gibbon_java_fibs_reader_alloc_name(_gibbon_java_fibs_reader_instance, \
	                                    s)
%}

%top{
#define yy_create_buffer gibbon_java_fibs_lexer_create_buffer
#define yy_delete_buffer gibbon_java_fibs_lexer_delete_buffer
#define yy_flush_buffer gibbon_java_fibs_lexer_flush_buffer
#define yy_scan_buffer gibbon_java_fibs_lexer_scan_buffer
#define yy_scan_bytes gibbon_java_fibs_lexer_scan_bytes
#define yy_scan_string gibbon_java_fibs_lexer_scan_string
#define yy_switch_to_buffer gibbon_java_fibs_lexer_switch_to_buffer
#define yyalloc gibbon_java_fibs_lexer_alloc
#define yyfree gibbon_java_fibs_lexer_free
#define yyget_debug gibbon_java_fibs_lexer_get_debug
#define yyget_in gibbon_java_fibs_lexer_get_in
#define yyget_leng gibbon_java_fibs_lexer_get_leng
#define yyget_lineno gibbon_java_fibs_lexer_get_lineno
#define yyget_out gibbon_java_fibs_lexer_get_out
#define yyget_text gibbon_java_fibs_lexer_get_text
#define yylex gibbon_java_fibs_lexer_lex
#define yylex_destroy gibbon_java_fibs_lexer_lex_destroy
#define yypop_buffer_state gibbon_java_fibs_lexer_pop_buffer_state
#define yypush_buffer_state gibbon_java_fibs_lexer_push_buffer_state
#define yyrealloc gibbon_java_fibs_lexer_realloc
#define yyrestart gibbon_java_fibs_lexer_restart
#define yyset_debug gibbon_java_fibs_lexer_set_debug
#define yyset_in gibbon_java_fibs_lexer_set_in
#define yyset_lineno gibbon_java_fibs_lexer_set_lineno
#define yyset_out gibbon_java_fibs_lexer_set_out
#define yy_flex_debug gibbon_java_fibs_lexer_flex_debug
#define yyin gibbon_java_fibs_lexer_in
#define yyleng gibbon_java_fibs_lexer_leng
#define yylineno gibbon_java_fibs_lexer_lineno
#define yyout gibbon_java_fibs_lexer_out
#define yytext gibbon_java_fibs_lexer_text
}

%option yylineno
%option nounput
%option noinput

/* %option debug */

%x OPCODE
%x WHO
%x WHAT

%%

  /***************************************************************************
   * General rules, valid for all scanner states.                            *
   ***************************************************************************/
   
  /* Ignore all whitespace.  */
<*>[ \t]			;
  /* Ignore carriage returns.  */
<*>\r				;

  /* 
   * The colon and the hyphen can never be part of a user name.  It is safe
   * to treat them the same in all scanner states.
   */
<*>:				return COLON;
<*>-				return HYPHEN;

  /***************************************************************************
   * In the initial state we are waiting for the format header but we ignore *
   * whitespace.                                                             *
   ***************************************************************************/
<INITIAL>\n				;
JavaFIBS[ \t]+internal[ \t]+match[ \t]+representation[ \t]+v[0-9.]+ {
					BEGIN (OPCODE);
					return PROLOG;
				}

  /***************************************************************************
   * After having read any valid line the scanner is in the OPCODE state     *
   * waiting for a decimal opcode in the range of 0-99.                      *
   ***************************************************************************/
  
<OPCODE>0			{
					BEGIN (WHO); 
					return ROLL;
				}
<OPCODE>1			{
					BEGIN (WHO); 
					return MOVE;
				}
<OPCODE>2			{
					BEGIN (WHO); 
					return CUBE;
				}
<OPCODE>3			{
					BEGIN (WHO); 
					return RESIGN;
				}
<OPCODE>4			{
					BEGIN (WHO); 
					return TAKE;
				}
<OPCODE>5			{
					BEGIN (WHO); 
					return DROP;
				}
<OPCODE>6			{
					BEGIN (WHO); 
					return OPPONENTS;
				}
<OPCODE>7			{
					BEGIN (WHO); 
					return WIN_GAME;
				}
<OPCODE>8			{
					BEGIN (WHO); 
					return START_OF_MATCH;
				}
<OPCODE>9			{
					BEGIN (WHO); 
					return WIN_MATCH;
				}
				
  /* I have yet to see that opcode in any of my matches.  Ignore it.  */
<OPCODE>10:[^\n]*		;

  /* Like 0 but for the opening roll.  We treat them the same */
<OPCODE>11 			{
					BEGIN (WHO);
					return ROLL;
				}
<OPCODE>12			{
					BEGIN (WHO); 
					return REJECT_RESIGN;
				}

  /* Always before 6 (OPPONENTS).  No semantics known.  */
<OPCODE>13			{
					BEGIN (WHO); 
					return START_OF_GAME;
				}
					
<OPCODE>14			{
					BEGIN (WHO); 
					return SCORE;
				}
					
  /* Cannot move or dance.  We can treat that as a regular (empty) move. */
<OPCODE>15			{
					BEGIN (WHO); 
					return MOVE;
				}
  /* Ignore all unknown opcodes.  */
<OPCODE>16:[^\n]*		;
<OPCODE>17:[^\n]*		;
<OPCODE>18:[^\n]*		;
<OPCODE>19:[^\n]*		;
<OPCODE>[2-9][0-9]:[^\n]*	;

  /*
   * Every newline terminates the current statement and returns to the  
   * OPCODE state.  That means that empty lines are effectively ignored.
   */
<OPCODE>\n			;

  /* Ouch! */
<OPCODE>.			return yytext[0];

  /***************************************************************************
   * After the opcode we expect a colon and then a player name.  Complaints  *
   * about a missing colon are left to the parser.                           *
   ***************************************************************************/
<WHO>[^-: \t\r\n]+		{
					BEGIN (WHAT);
					yylval.name = alloc_name (yytext);
					return PLAYER;
				}
<WHO>.|\n			return yytext[0];

  /***************************************************************************
   * What is following after the player name, is opcode-specific but         *
   * fortunately we can identify all valid tokens without context.           *
   ***************************************************************************/

<WHAT>bar			return BAR;
<WHAT>o(ff)?			return OFF;
<WHAT>[1-9][0-9]*		{
					/* We leave all complaints
					 * about conversion errors to 
					 * the higher-level analysis.  
					 */
					yylval.num = (gint) g_ascii_strtoull 
							     (yytext, NULL, 10); 
					return INTEGER;
				}
<WHAT>0				{
					yylval.num = 0; 
					return INTEGER;
				}
<WHAT>[^-: \t\r\n]+		{
					BEGIN (WHAT);
					yylval.name = alloc_name (yytext);
					return PLAYER;
				}
<WHAT>\n			BEGIN (OPCODE);
.				return yytext[0];
%%

static int
yywrap ()
{
	return 1;
}