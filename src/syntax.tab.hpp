/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEG = 258,
     TIC = 259,
     IN = 260,
     INOUT = 261,
     OUT = 262,
     VECTOR = 263,
     MATRIX = 264,
     TENSOR = 265,
     SCALAR = 266,
     SQUAREROOT = 267,
     ORIENTATION = 268,
     ROW = 269,
     COLUMN = 270,
     CONTAINER = 271,
     FORMAT = 272,
     GENERAL = 273,
     TRIANGULAR = 274,
     UPLO = 275,
     UPPER = 276,
     LOWER = 277,
     DIAG = 278,
     UNIT = 279,
     NONUNIT = 280,
     VAR = 281,
     NUM = 282
   };
#endif
/* Tokens.  */
#define NEG 258
#define TIC 259
#define IN 260
#define INOUT 261
#define OUT 262
#define VECTOR 263
#define MATRIX 264
#define TENSOR 265
#define SCALAR 266
#define SQUAREROOT 267
#define ORIENTATION 268
#define ROW 269
#define COLUMN 270
#define CONTAINER 271
#define FORMAT 272
#define GENERAL 273
#define TRIANGULAR 274
#define UPLO 275
#define UPPER 276
#define LOWER 277
#define DIAG 278
#define UNIT 279
#define NONUNIT 280
#define VAR 281
#define NUM 282




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 37 "syntax.y"
{
  expr* expression;
  stmt* statement;
  std::vector<stmt*>* program;
  std::string* variable;
  param* parameter;
  double number;
  std::map<string,type*>* parameter_list;
  type* value_type;
  storage store;
  std::string *orien;
  attrib *attribute;
  std::map<string,string>* attribute_list;
}
/* Line 1529 of yacc.c.  */
#line 118 "syntax.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

