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
 * Parser for the internal format of JavaFIBS.  This grammar describes that
 * format only loosely.  This is sufficient for parsing because we will
 * ignore all redundant date, while building the syntax tree.
 */

%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include "gibbon-java-fibs-parser.h"
#include "gibbon-java-fibs-reader.h"

/*
 * Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
 * as well as gratuitiously global symbol names, so we can have multiple
 * yacc generated parsers in the same program.  Note that these are only
 * the variables produced by yacc.  If other parser generators (bison,
 * byacc, etc) produce additional global names that conflict at link time,
 * then those parser generators need to be fixed instead of adding those
 * names to this list. 
 */

#define yymaxdepth gibbon_java_fibs_parser_maxdepth
#define yyparse    gibbon_java_fibs_parser_parse
#define yylex      gibbon_java_fibs_lexer_lex
extern int gibbon_java_fibs_lexer_lex (void);
#define yyerror    gibbon_java_fibs_reader_yyerror
#define yylval     gibbon_java_fibs_parser_lval
#define yychar     gibbon_java_fibs_parser_char
#define yydebug    gibbon_java_fibs_parser_debug
#define yypact     gibbon_java_fibs_parser_pact
#define yyr1       gibbon_java_fibs_parser_r1
#define yyr2       gibbon_java_fibs_parser_r2
#define yydef      gibbon_java_fibs_parser_def
#define yychk      gibbon_java_fibs_parser_chk
#define yypgo      gibbon_java_fibs_parser_pgo
#define yyact      gibbon_java_fibs_parser_act
#define yyexca     gibbon_java_fibs_parser_exca
#define yyerrflag  gibbon_java_fibs_parser_errflag
#define yynerrs    gibbon_java_fibs_parser_nerrs
#define yyps       gibbon_java_fibs_parser_ps
#define yypv       gibbon_java_fibs_parser_pv
#define yys        gibbon_java_fibs_parser_s
#define yy_yys     gibbon_java_fibs_parser_yys
#define yystate    gibbon_java_fibs_parser_state
#define yytmp      gibbon_java_fibs_parser_tmp
#define yyv        gibbon_java_fibs_parser_v
#define yy_yyv     gibbon_java_fibs_parser_yyv
#define yyval      gibbon_java_fibs_parser_val
#define yylloc     gibbon_java_fibs_parser_lloc
#define yyreds     gibbon_java_fibs_parser_reds    /* With YYDEBUG defined */
#define yytoks     gibbon_java_fibs_parser_toks    /* With YYDEBUG defined */
#define yylhs      gibbon_java_fibs_parser_yylhs
#define yylen      gibbon_java_fibs_parser_yylen
#define yydefred   gibbon_java_fibs_parser_yydefred
#define yysdgoto    gibbon_java_fibs_parser_yydgoto
#define yysindex   gibbon_java_fibs_parser_yysindex
#define yyrindex   gibbon_java_fibs_parser_yyrindex
#define yygindex   gibbon_java_fibs_parser_yygindex
#define yytable    gibbon_java_fibs_parser_yytable
#define yycheck    gibbon_java_fibs_parser_yycheck
%}

%union {
	gint num;
	gchar *name;
}

%token PROLOG
%token COLON
%token HYPHEN
%token <num> INTEGER
%token <name> PLAYER
%token ROLL
%token MOVE
%token CUBE
%token RESIGN
%token TAKE
%token DROP
%token START_OF_GAME
%token WIN_GAME
%token START_OF_MATCH
%token WIN_MATCH
%token REJECT_RESIGN
%token OPPONENTS
%token SCORE
%token BAR
%token OFF
%token JUNK

%%

java_fibs_file
        : PROLOG match
        ;

match
	: START_OF_MATCH COLON PLAYER COLON INTEGER games
	;

games
	: /* empty */
	| games game
	;

game
	: start_of_game opponents actions
	;

start_of_game
	: START_OF_GAME COLON PLAYER COLON
	;

opponents
	: OPPONENTS COLON PLAYER COLON PLAYER
	;

actions
	: /* empty */
	| actions action
	;
	
action
	: roll | move | cube | drop | take | score 
	| resign | reject_resign | win_game | win_match
	;

roll
	: ROLL COLON PLAYER COLON INTEGER INTEGER
	;

move
	: MOVE COLON PLAYER COLON movements
	;

movements
	: /* empty */
	| movement
	| movement movement
	| movement movement movement
	| movement movement movement movement
	;

movement
	: point HYPHEN point
	;

point
	: INTEGER | BAR | OFF
	;

cube
	: CUBE COLON PLAYER COLON
	;

drop
	: DROP COLON PLAYER COLON
	;

take
	: TAKE COLON PLAYER COLON
	;
	
win_game
	: WIN_GAME COLON PLAYER COLON INTEGER
	;
	
score
	: SCORE COLON PLAYER HYPHEN INTEGER COLON PLAYER HYPHEN INTEGER
	;

resign
	: RESIGN COLON PLAYER COLON INTEGER
	;
	
reject_resign
	: REJECT_RESIGN COLON PLAYER COLON
	;
	
win_match
	: WIN_MATCH COLON PLAYER COLON INTEGER
	;

%%