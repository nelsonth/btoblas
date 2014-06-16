/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 2 "syntax.y"


#include <iostream>
#include <fstream>
#include <map>
#include "boost/lexical_cast.hpp"
#include "syntax.hpp"
#include "syntax.tab.hpp"
#include "compile.hpp"
#include "boost/program_options.hpp"
#include <string>

int yylex();
extern int yylineno;
void yyerror(std::string s);

std::string out_file_name;
std::string input_file;
extern FILE *yyin; 
boost::program_options::variables_map vm;

    void handleError(std::string errorMsg);
    extern int bto_yy_input(char *b, int maxBuffer);
    
    static int eof = 0;
    static int nRow = 0;
    static int nBuffer = 0;
    static int lBuffer = 0;
    static int nTokenStart = 0;
    static int nTokenLength = 0;
    static int nTokenNextStart = 0;
    static int lMaxBuffer = 1000;
    static char *buffer;


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
/* Line 193 of yacc.c.  */
#line 200 "syntax.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 213 "syntax.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   162

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  41
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  10
/* YYNRULES -- Number of rules.  */
#define YYNRULES  60
/* YYNRULES -- Number of states.  */
#define YYNSTATES  125

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   282

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    38,
      35,    36,    30,    29,    34,    28,     2,    39,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    33,     2,
       2,    37,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    31,    40,    32,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,    14,    23,    32,    41,    46,    51,    58,
      65,    74,    81,    85,    89,    93,    97,    99,   103,   107,
     112,   117,   122,   124,   127,   130,   133,   137,   139,   143,
     147,   149,   153,   157,   159,   161,   165,   169,   171,   173,
     177,   181,   183,   185,   187,   191,   192,   195,   198,   202,
     206,   208,   210,   214,   218,   222,   225,   228,   232,   237,
     241
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      42,     0,    -1,    26,     5,    44,     6,    44,     7,    44,
      31,    48,    32,    -1,    26,     5,    44,     6,    44,    31,
      48,    32,    -1,    26,     6,    44,     7,    44,    31,    48,
      32,    -1,    26,     5,    44,     7,    44,    31,    48,    32,
      -1,    26,     5,     1,     6,    -1,    26,     5,     1,     7,
      -1,    26,     5,    44,     6,     1,     7,    -1,    26,     5,
      44,     6,     1,    31,    -1,    26,     5,    44,     6,    44,
       7,     1,    31,    -1,    26,     5,    44,     7,     1,    31,
      -1,    26,     1,    31,    -1,    26,    33,    45,    -1,    26,
      33,     1,    -1,    26,     1,    45,    -1,    43,    -1,    44,
      34,    43,    -1,    44,    34,     1,    -1,     9,    35,    47,
      36,    -1,     8,    35,    47,    36,    -1,    10,    35,    47,
      36,    -1,    11,    -1,     9,     1,    -1,     8,     1,    -1,
      10,     1,    -1,    13,    37,    14,    -1,    14,    -1,    13,
      37,    15,    -1,    16,    37,    26,    -1,    15,    -1,    17,
      37,    18,    -1,    17,    37,    19,    -1,    18,    -1,    19,
      -1,    20,    37,    21,    -1,    20,    37,    22,    -1,    21,
      -1,    22,    -1,    23,    37,    24,    -1,    23,    37,    25,
      -1,    24,    -1,    25,    -1,    46,    -1,    47,    34,    46,
      -1,    -1,    48,    49,    -1,    48,     1,    -1,    26,    37,
      50,    -1,    26,    37,     1,    -1,    27,    -1,    26,    -1,
      50,    29,    50,    -1,    50,    28,    50,    -1,    50,    30,
      50,    -1,    28,    50,    -1,    50,    38,    -1,    35,    50,
      36,    -1,    12,    35,    50,    36,    -1,    50,    39,    50,
      -1,    40,    50,    40,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    76,    76,    78,    81,    84,    87,    88,    89,    90,
      91,    92,    93,    95,    96,    97,    99,   100,   101,   103,
     104,   105,   106,   107,   108,   109,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   129,   130,   132,   133,   134,   136,   137,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NEG", "TIC", "IN", "INOUT", "OUT",
  "VECTOR", "MATRIX", "TENSOR", "SCALAR", "SQUAREROOT", "ORIENTATION",
  "ROW", "COLUMN", "CONTAINER", "FORMAT", "GENERAL", "TRIANGULAR", "UPLO",
  "UPPER", "LOWER", "DIAG", "UNIT", "NONUNIT", "VAR", "NUM", "'-'", "'+'",
  "'*'", "'{'", "'}'", "':'", "','", "'('", "')'", "'='", "'''", "'/'",
  "'|'", "$accept", "input", "param", "param_list", "type", "attrib",
  "attrib_list", "prog", "stmt", "expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,    45,    43,
      42,   123,   125,    58,    44,    40,    41,    61,    39,    47,
     124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    41,    42,    42,    42,    42,    42,    42,    42,    42,
      42,    42,    42,    43,    43,    43,    44,    44,    44,    45,
      45,    45,    45,    45,    45,    45,    46,    46,    46,    46,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    47,    47,    48,    48,    48,    49,    49,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
      50
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,    10,     8,     8,     8,     4,     4,     6,     6,
       8,     6,     3,     3,     3,     3,     1,     3,     3,     4,
       4,     4,     1,     2,     2,     2,     3,     1,     3,     3,
       1,     3,     3,     1,     1,     3,     3,     1,     1,     3,
       3,     1,     1,     1,     3,     0,     2,     2,     3,     3,
       1,     1,     3,     3,     3,     2,     2,     3,     4,     3,
       3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     1,    12,     0,     0,
      16,     0,     0,     6,     7,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    22,    15,    14,    13,     0,     0,
       0,     0,    18,    17,     0,    24,     0,    23,     0,    25,
       0,     8,     9,     0,    45,    11,    45,    45,     0,    27,
      30,     0,     0,    33,    34,     0,    37,    38,     0,    41,
      42,    43,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    20,    19,    21,    10,
      45,    47,     0,     3,    46,     5,     4,    26,    28,    29,
      31,    32,    35,    36,    39,    40,    44,     0,     0,     2,
      49,     0,    51,    50,     0,     0,     0,    48,     0,    55,
       0,     0,     0,     0,     0,    56,     0,     0,    57,    60,
      53,    52,    54,    59,    58
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,    10,    11,    25,    61,    62,    67,    84,   107
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -44
static const yytype_int16 yypact[] =
{
     -10,   129,    54,    70,    52,    59,   -44,   -44,   139,     5,
     -44,    22,    24,   -44,   -44,   128,   118,    56,    58,    60,
      59,     0,     6,     7,   -44,   -44,   -44,   -44,     2,    45,
     100,    46,   -44,   -44,    68,   -44,    50,   -44,    50,   -44,
      50,   -44,   -44,    61,   -44,   -44,   -44,   -44,    72,   -44,
     -44,    88,   110,   -44,   -44,   119,   -44,   -44,   120,   -44,
     -44,   -44,   -24,    47,    79,   127,   109,     4,    17,    18,
       9,   115,   130,   131,   126,    50,   -44,   -44,   -44,   -44,
     -44,   -44,   122,   -44,   -44,   -44,   -44,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,   -44,   -44,   -44,    19,    -1,   -44,
     -44,   125,   -44,   -44,    20,    20,    20,    94,    20,   116,
      78,    65,    20,    20,    20,   -44,    20,    82,   -44,   -44,
     -17,   -17,   116,    94,   -44
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -44,   -44,   142,    -3,    75,    87,   104,   -43,   -44,   -16
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
     100,    35,    12,    68,    69,    81,    15,    37,    39,    41,
      75,   101,    76,   114,    29,    31,     1,    34,    81,    81,
      81,   115,   116,    87,    88,   102,   103,   104,    17,    18,
      82,    20,   101,    42,   105,    36,    83,    97,    16,   106,
      66,    38,    40,    82,    82,    82,   102,   103,   104,    85,
      86,    99,    43,     8,     6,   105,    19,    28,    19,    30,
     106,    32,    65,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    44,    46,     9,    19,
      19,    75,     9,    77,     9,     9,     9,     9,   109,   110,
     111,    27,   117,   112,   113,   114,   120,   121,   122,    47,
     123,     7,    19,   115,   116,   119,   112,   113,   114,    70,
     112,   113,   114,    75,   118,    78,   115,   116,   124,    26,
     115,   116,   112,   113,   114,    71,    21,    22,    23,    24,
       3,    45,   115,   116,     4,     5,    21,    22,    23,    24,
      80,    89,    63,    19,    64,    13,    14,    72,    90,    91,
      94,    95,    92,    93,   115,   116,    73,    74,    79,    98,
     108,    33,    96
};

static const yytype_uint8 yycheck[] =
{
       1,     1,     5,    46,    47,     1,     1,     1,     1,     7,
      34,    12,    36,    30,    17,    18,    26,    20,     1,     1,
       1,    38,    39,    14,    15,    26,    27,    28,     6,     7,
      26,     7,    12,    31,    35,    35,    32,    80,    33,    40,
      43,    35,    35,    26,    26,    26,    26,    27,    28,    32,
      32,    32,     7,     1,     0,    35,    34,     1,    34,     1,
      40,     1,     1,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    31,    31,    26,    34,
      34,    34,    26,    36,    26,    26,    26,    26,   104,   105,
     106,    16,   108,    28,    29,    30,   112,   113,   114,    31,
     116,    31,    34,    38,    39,    40,    28,    29,    30,    37,
      28,    29,    30,    34,    36,    36,    38,    39,    36,     1,
      38,    39,    28,    29,    30,    37,     8,     9,    10,    11,
       1,    31,    38,    39,     5,     6,     8,     9,    10,    11,
      31,    26,    38,    34,    40,     6,     7,    37,    18,    19,
      24,    25,    21,    22,    38,    39,    37,    37,    31,    37,
      35,    19,    75
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    26,    42,     1,     5,     6,     0,    31,     1,    26,
      43,    44,    44,     6,     7,     1,    33,     6,     7,    34,
       7,     8,     9,    10,    11,    45,     1,    45,     1,    44,
       1,    44,     1,    43,    44,     1,    35,     1,    35,     1,
      35,     7,    31,     7,    31,    31,    31,    31,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    46,    47,    47,    47,     1,    44,    48,    48,    48,
      37,    37,    37,    37,    37,    34,    36,    36,    36,    31,
      31,     1,    26,    32,    49,    32,    32,    14,    15,    26,
      18,    19,    21,    22,    24,    25,    46,    48,    37,    32,
       1,    12,    26,    27,    28,    35,    40,    50,    35,    50,
      50,    50,    28,    29,    30,    38,    39,    50,    36,    40,
      50,    50,    50,    50,    36
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 77 "syntax.y"
    { compile(vm, out_file_name, *(yyvsp[(1) - (10)].variable), *(yyvsp[(3) - (10)].parameter_list), *(yyvsp[(5) - (10)].parameter_list), *(yyvsp[(7) - (10)].parameter_list), *(yyvsp[(9) - (10)].program)); ;}
    break;

  case 3:
#line 79 "syntax.y"
    { std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *(yyvsp[(1) - (8)].variable), *(yyvsp[(3) - (8)].parameter_list), *(yyvsp[(5) - (8)].parameter_list), *tmp, *(yyvsp[(7) - (8)].program)); ;}
    break;

  case 4:
#line 82 "syntax.y"
    { std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *(yyvsp[(1) - (8)].variable), *tmp, *(yyvsp[(3) - (8)].parameter_list), *(yyvsp[(5) - (8)].parameter_list), *(yyvsp[(7) - (8)].program)); ;}
    break;

  case 5:
#line 85 "syntax.y"
    { std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *(yyvsp[(1) - (8)].variable), *(yyvsp[(3) - (8)].parameter_list), *tmp, *(yyvsp[(5) - (8)].parameter_list), *(yyvsp[(7) - (8)].program)); ;}
    break;

  case 6:
#line 87 "syntax.y"
    { handleError("Error parsing input variables"); ;}
    break;

  case 7:
#line 88 "syntax.y"
    { handleError("Error parsing input variables"); ;}
    break;

  case 8:
#line 89 "syntax.y"
    { handleError("Error parsing inout variables"); ;}
    break;

  case 9:
#line 90 "syntax.y"
    { handleError("Error parsing inout variables"); ;}
    break;

  case 10:
#line 91 "syntax.y"
    { handleError("Error parsing out variables"); ;}
    break;

  case 11:
#line 92 "syntax.y"
    { handleError("Error parsing out variables"); ;}
    break;

  case 12:
#line 93 "syntax.y"
    { handleError("Error parsing out variables"); ;}
    break;

  case 13:
#line 95 "syntax.y"
    { (yyval.parameter) = new param(*(yyvsp[(1) - (3)].variable),(yyvsp[(3) - (3)].value_type)); ;}
    break;

  case 14:
#line 96 "syntax.y"
    { handleError("Error parsing variable declaration"); ;}
    break;

  case 15:
#line 97 "syntax.y"
    { handleError("Error: missing \':\' in this declaration"); ;}
    break;

  case 16:
#line 99 "syntax.y"
    { (yyval.parameter_list) = new std::map<string,type*>(); (yyval.parameter_list)->insert(*(yyvsp[(1) - (1)].parameter)); ;}
    break;

  case 17:
#line 100 "syntax.y"
    { (yyval.parameter_list) = (yyvsp[(1) - (3)].parameter_list); (yyval.parameter_list)->insert(*(yyvsp[(3) - (3)].parameter)); ;}
    break;

  case 18:
#line 101 "syntax.y"
    { handleError("Error parsing parameters"); ;}
    break;

  case 19:
#line 103 "syntax.y"
    { (yyval.value_type) = new type("matrix",*(yyvsp[(3) - (4)].attribute_list)); ;}
    break;

  case 20:
#line 104 "syntax.y"
    { (yyval.value_type) = new type("vector",*(yyvsp[(3) - (4)].attribute_list)); ;}
    break;

  case 21:
#line 105 "syntax.y"
    { (yyval.value_type) = new type("tensor",*(yyvsp[(3) - (4)].attribute_list)); ;}
    break;

  case 22:
#line 106 "syntax.y"
    { (yyval.value_type) = new type(scalar); ;}
    break;

  case 23:
#line 107 "syntax.y"
    { handleError("Error parsing matrix attributes"); ;}
    break;

  case 24:
#line 108 "syntax.y"
    { handleError("Error parsing vector attributes");;}
    break;

  case 25:
#line 109 "syntax.y"
    { handleError("Error parsing vector attributes");;}
    break;

  case 26:
#line 111 "syntax.y"
    { (yyval.attribute) = new attrib("orientation","row"); ;}
    break;

  case 27:
#line 112 "syntax.y"
    { (yyval.attribute) = new attrib("orientation","row"); ;}
    break;

  case 28:
#line 113 "syntax.y"
    { (yyval.attribute) = new attrib("orientation","column"); ;}
    break;

  case 29:
#line 114 "syntax.y"
    { (yyval.attribute) = new attrib("container", *(yyvsp[(3) - (3)].variable)); ;}
    break;

  case 30:
#line 115 "syntax.y"
    { (yyval.attribute) = new attrib("orientation","column"); ;}
    break;

  case 31:
#line 116 "syntax.y"
    { (yyval.attribute) = new attrib("format","general"); ;}
    break;

  case 32:
#line 117 "syntax.y"
    { (yyval.attribute) = new attrib("format","triangular"); ;}
    break;

  case 33:
#line 118 "syntax.y"
    { (yyval.attribute) = new attrib("format","general"); ;}
    break;

  case 34:
#line 119 "syntax.y"
    { (yyval.attribute) = new attrib("format","triangular"); ;}
    break;

  case 35:
#line 120 "syntax.y"
    { (yyval.attribute) = new attrib("uplo","upper"); ;}
    break;

  case 36:
#line 121 "syntax.y"
    { (yyval.attribute) = new attrib("uplo","lower"); ;}
    break;

  case 37:
#line 122 "syntax.y"
    { (yyval.attribute) = new attrib("uplo","upper"); ;}
    break;

  case 38:
#line 123 "syntax.y"
    { (yyval.attribute) = new attrib("uplo","lower"); ;}
    break;

  case 39:
#line 124 "syntax.y"
    { (yyval.attribute) = new attrib("diag","unit"); ;}
    break;

  case 40:
#line 125 "syntax.y"
    { (yyval.attribute) = new attrib("diag","nonunit"); ;}
    break;

  case 41:
#line 126 "syntax.y"
    { (yyval.attribute) = new attrib("diag","unit"); ;}
    break;

  case 42:
#line 127 "syntax.y"
    { (yyval.attribute) = new attrib("diag","nonunit"); ;}
    break;

  case 43:
#line 129 "syntax.y"
    { (yyval.attribute_list) = new std::map<string,string>(); (yyval.attribute_list)->insert(*(yyvsp[(1) - (1)].attribute)); ;}
    break;

  case 44:
#line 130 "syntax.y"
    { (yyval.attribute_list) = (yyvsp[(1) - (3)].attribute_list); (yyval.attribute_list)->insert(*(yyvsp[(3) - (3)].attribute)); ;}
    break;

  case 45:
#line 132 "syntax.y"
    { (yyval.program) = new std::vector<stmt*>(); ;}
    break;

  case 46:
#line 133 "syntax.y"
    { (yyval.program) = (yyvsp[(1) - (2)].program); (yyval.program)->push_back((yyvsp[(2) - (2)].statement)); ;}
    break;

  case 47:
#line 134 "syntax.y"
    {handleError("Error in program body"); ;}
    break;

  case 48:
#line 136 "syntax.y"
    { (yyval.statement) = new stmt((yyvsp[(1) - (3)].variable),(yyvsp[(3) - (3)].expression)); ;}
    break;

  case 49:
#line 137 "syntax.y"
    { handleError("Error parsing operations"); ;}
    break;

  case 50:
#line 139 "syntax.y"
    { (yyval.expression) = new scalar_in((yyvsp[(1) - (1)].number)); ;}
    break;

  case 51:
#line 140 "syntax.y"
    { (yyval.expression) = new variable((yyvsp[(1) - (1)].variable)); ;}
    break;

  case 52:
#line 141 "syntax.y"
    { (yyval.expression) = new operation(add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); ;}
    break;

  case 53:
#line 142 "syntax.y"
    { (yyval.expression) = new operation(subtract, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); ;}
    break;

  case 54:
#line 143 "syntax.y"
    { (yyval.expression) = new operation(multiply, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); ;}
    break;

  case 55:
#line 144 "syntax.y"
    { (yyval.expression) = new operation(negate_op, (yyvsp[(2) - (2)].expression)); ;}
    break;

  case 56:
#line 145 "syntax.y"
    { (yyval.expression) = new operation(trans, (yyvsp[(1) - (2)].expression)); ;}
    break;

  case 57:
#line 146 "syntax.y"
    { (yyval.expression) = (yyvsp[(2) - (3)].expression); ;}
    break;

  case 58:
#line 147 "syntax.y"
    { (yyval.expression) = new operation(squareroot, (yyvsp[(3) - (4)].expression)); ;}
    break;

  case 59:
#line 148 "syntax.y"
    { (yyval.expression) = new operation(divide, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression)); ;}
    break;

  case 60:
#line 149 "syntax.y"
    { (yyval.expression) = new operation(squareroot, new operation(multiply, new operation(trans, (yyvsp[(2) - (3)].expression)), (yyvsp[(2) - (3)].expression))); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 1818 "syntax.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 151 "syntax.y"

int main(int argc, char *argv[]) 
{
    // buffer to keep current line copy while parsing for 
    // improved error handling
    buffer = (char*)malloc(lMaxBuffer);
    if (buffer == NULL) {
        printf("cannot allocate %d bytes of memory\n", lMaxBuffer);
        fclose(yyin);
        return 12;
    }
    
  namespace po = boost::program_options;
  
  string config_file;
  
  po::options_description cmdOnly("Command Line Only Options");
  cmdOnly.add_options() 
        ("help,h","this help message")
        ("config,f", po::value<std::string>(&config_file)->default_value(""),
                "Name of configuration file.")
        ;
  
  po::options_description vis("Options");
  vis.add_options()
    
		("precision,a",po::value<std::string>()->default_value("double"),
  				"Set precision type for generated output [float|double]\n"
  				"  double is default")
		("empirical_off,e","Disable empirical testing.  Empirical testing\n"
				"is enabled by default.")
		("correctness,c","Enable correctness testing.  Correctness testing\n"
				"is disabled by default.  Enabling requires a BLAS library\n"
				"to be present.  (Set path in top level make.inc. See \n"
				"documentation for further details")
		("use_model,m","Enable the analytic model when testing kernels. If "
				"set the compiler will use a memory model to help with "
				"optimization choices.  Not recommended for most users. ")
		("threshold,t",po::value<double>()->default_value(0.01),
				"This parameter controls how much empirical testing is "
				 "performed. For example, the default of 0.01 says that "
				"empirical testing will be performed on any version "
				"that is predicted to be within %1 the performance of "
				"the best predicted version.  A value of 1 will "
				"rank all versions. A value of 0 will select only the "
				"best version. Only used if use_model is on.")
		//("level1",po::value<std::string>()->default_value("thread 2:12:2"),
		("level1",po::value<std::string>()->default_value(""),
				"Choose thread or cache tiling for outer level, "
				"and input parameter search range. "
				"Example \"thread 2:12:2\" or \"cache 64:512:8\".")
		("level2",po::value<std::string>()->default_value(""),
				"Choose thread or cache tiling for inner level, "
				"and input parameter search range. "
				"Example \"thread 2:12:2\" or \"cache 64:512:8\".")
		("test_param,r",po::value<std::string>()->default_value("3000:3000:1"),
  				"Set parameters for empirical and correctness tests as\n"
  				"start:stop:step")
		("search,s",po::value<std::string>()->default_value("ga"),
				"select the search strategy:\n"
				"  [ga|ex|random|orthogonal|debug|thread] \n"
				"    ga is the default \n"
				" ga: genetic algorithm \n"
				"  orthogonal: \tsearch one parameter completely first, then"
				" search next.  For example find best fusion, and then find"
				" best partition strategy\n"
				"  random: \trandomly search the space\n"
				"  exhaustive: \texhaustively search the space (very long run times)\n"
				"thread: thread search only \n"
				"debug: use the versions specified in debug.txt \n")
		("ga_timelimit",po::value<int>()->default_value(10),
				"Specifies the number of GA versions to be tested")
		("empirical_reps",po::value<int>()->default_value(5),
				"the number of empirical test repetitions for each point")
        ("delete_tmp","delete *_tmp directory without prompting")
		("ga_popsize",po::value<int>()->default_value(10),"population size")
		("ga_nomaxfuse","genetic algorithm without max fuse")
		("ga_noglobalthread","genetic algorithm without global thread search")
		("ga_exthread", "genetic algorithm with additional exhaustive thread search")
  		;
  
  po::options_description hidden("Hidden options");
  hidden.add_options()
		("distributed,d","Enable distributed calculation via MPI.")
  		("input-file", po::value<std::string>(), "input file")
		("run_all","allow compiler to run with model and empirical tests off")
		("limit,l",po::value<int>()->default_value(-1),
				"Specifies a time limit in minutes for the search "
				"Default is unlimited time.")
		("partition_off,p","Disable partitioning.  Enabled by default, generates\n"
				"parallel code that requires a Pthreads library.")
		("backend,b",po::value<std::string>()->default_value("ptr"),
				"select the code generation backend:\n"
				"  [ptr|noptr] \tptr is default\n"
				"  ptr: \tproduce c code using pointers\n"
				"  noptr: \tproduce c code using variable length arrays\n"
				"\nSelecting noptr requires partitioning to be disabled")
  		;
  po::options_description cmdline_ops;
  cmdline_ops.add(cmdOnly).add(vis).add(hidden);
  
  po::options_description config_file_options;
  config_file_options.add(vis).add(hidden);
  
  po::positional_options_description p;
  p.add("input-file", -1);
  
  try {
    
  po::store(po::command_line_parser(argc,argv).options(cmdline_ops).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help") || !(vm.count("input-file"))) {
  	std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
  	return 1;
  }	
  
  if (!config_file.empty()) {
	  std::fstream ifs(config_file.c_str());
	  if (!ifs) {
		std::cout << "Can not open config file: " << config_file << "\n";
		return 1;
	  } else {
		store(parse_config_file(ifs, config_file_options), vm);
	  }
  }
   
  if (!(vm["precision"].as<std::string>().compare("float") == 0 ||
  			vm["precision"].as<std::string>().compare("double") == 0)) {
  		std::cout << "ERROR:" << std::endl;
  		std::cout << "\t" << vm["precision"].as<std::string>() << " is not a valid "
  				  << "option for --precision (-p)" << std::endl;
  		std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis
  				  << std::endl;
  		return 1;		
  }

  if (!(vm["backend"].as<std::string>().compare("ptr") == 0 ||
			vm["backend"].as<std::string>().compare("noptr") == 0)) {
		std::cout << "ERROR:" << std::endl;
        std::cout << "\t" << vm["backend"].as<std::string>() << " is not a valid option "
                  << "for --backend (-b)" << std::endl;
        std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
        return 1;

  }
  
  if ((vm["backend"].as<std::string>().compare("noptr") == 0) && !vm.count("partition_off")) {
	std::cout << "ERROR:" << std::endl;
    std::cout << "\t" << "Cannot enable partitioning with noptr backend selected.\n";
    std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
    return 1;
  }

	//if (vm.count("model_off") && vm.count("empirical_off")  && !vm.count("run_all")) {
		//std::cout << "ERROR:" << std::endl;
	    //std::cout << "\t" << "Either model or empirical testing or both required.\n";
	    //std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
	    //return 1;
	//}

  input_file = vm["input-file"].as<std::string>();
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
    return 1;
  }
  
  out_file_name = input_file;
  size_t loc = out_file_name.rfind(".m");
  if (loc != string::npos)
  	out_file_name.replace(loc,2,"");
  else
  	std::cout << "WARNING:\n\tinput file \"" <<  input_file << "\" may not be a .m file\n";
  
  yyin = fopen(input_file.c_str(), "r");
  if (yyin == NULL) {
  	std::cout << "ERROR:\n\tunable to open \"" << input_file << "\"" << std::endl;;
  	std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
  	return 1;	
  } 
  
  yyparse(); 
  
  fclose(yyin);
  return 0;
} 
void yyerror (std::string s) /* Called by yyparse on error */ 
{ 
  std::cout << "line " << yylineno << ": " << s << std::endl;
} 

void handleError(std::string errorMsg) {    
    
    int start=nTokenStart;
    int end=start + nTokenLength - 1;
    int i;
    std::cout << errorMsg << std::endl;
    std::cout << buffer;
    
    if (!eof) {
        for (i=1; i<start; i++)
            std::cout << ".";
        for (i=start; i<=end; i++)
            std::cout << "^";
        for (i=end+1; i<lBuffer; i++)
        std::cout << ".";
        std::cout << std::endl;
    }
    
    exit(-1);
}

static int getNextLine(void) {
    
    // reset global variables storing local buffer information.
    // the local buffer is used to that bto can better report errors
    // encountered when parsing.
    nBuffer = 0;
    nTokenStart = -1;
    nTokenNextStart = 1;
    eof = 0;
    
    // read next line from input file.  if not eof or other error
    // store line in global buffer.
    char *p = fgets(buffer, lMaxBuffer, yyin);
    if (p == NULL) {
        if (ferror(yyin))
            return -1;
        eof = true;
        return 1;
    }
    
    nRow += 1;
    lBuffer = strlen(buffer);
    
    return 0;
}

extern int bto_yy_input(char *b, int maxBuffer) {
    // custom reader so bto can track errors better
    // must read next maxBuffer characeters from input file (global yyin)
    // into buffer b.  This will return number of characters read
    // or 0 for no characters of eof.
    
    if (eof)
        return 0;
    
    while (nBuffer >= lBuffer) {
        if (getNextLine())
            return 0;
    }
    
    b[0] = buffer[nBuffer];
    nBuffer += 1;
    
    return b[0]==0?0:1;
}

extern void BeginToken(char *t) {
    // each time a token is encountered, stored the current location
    nTokenStart = nTokenNextStart;
    nTokenLength = strlen(t);
    nTokenNextStart = nBuffer + 1;
}

