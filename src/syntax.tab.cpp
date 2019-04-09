/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 2 "syntax.y" /* yacc.c:339  */


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

#line 101 "syntax.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "syntax.tab.h".  */
#ifndef YY_YY_SYNTAX_TAB_H_INCLUDED
# define YY_YY_SYNTAX_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 37 "syntax.y" /* yacc.c:355  */

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

#line 184 "syntax.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SYNTAX_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 201 "syntax.tab.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

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
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  125

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   282

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
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

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NEG", "TIC", "IN", "INOUT", "OUT",
  "VECTOR", "MATRIX", "TENSOR", "SCALAR", "SQUAREROOT", "ORIENTATION",
  "ROW", "COLUMN", "CONTAINER", "FORMAT", "GENERAL", "TRIANGULAR", "UPLO",
  "UPPER", "LOWER", "DIAG", "UNIT", "NONUNIT", "VAR", "NUM", "'-'", "'+'",
  "'*'", "'{'", "'}'", "':'", "','", "'('", "')'", "'='", "'\\''", "'/'",
  "'|'", "$accept", "input", "param", "param_list", "type", "attrib",
  "attrib_list", "prog", "stmt", "expr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,    45,    43,
      42,   123,   125,    58,    44,    40,    41,    61,    39,    47,
     124
};
# endif

#define YYPACT_NINF -44

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-44)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
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

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
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

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -44,   -44,   142,    -3,    75,    87,   104,   -43,   -44,   -16
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,    10,    11,    25,    61,    62,    67,    84,   107
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
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

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 77 "syntax.y" /* yacc.c:1646  */
    { compile(vm, out_file_name, *(yyvsp[-9].variable), *(yyvsp[-7].parameter_list), *(yyvsp[-5].parameter_list), *(yyvsp[-3].parameter_list), *(yyvsp[-1].program)); }
#line 1364 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 79 "syntax.y" /* yacc.c:1646  */
    { std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *(yyvsp[-7].variable), *(yyvsp[-5].parameter_list), *(yyvsp[-3].parameter_list), *tmp, *(yyvsp[-1].program)); }
#line 1371 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 82 "syntax.y" /* yacc.c:1646  */
    { std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *(yyvsp[-7].variable), *tmp, *(yyvsp[-5].parameter_list), *(yyvsp[-3].parameter_list), *(yyvsp[-1].program)); }
#line 1378 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 85 "syntax.y" /* yacc.c:1646  */
    { std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *(yyvsp[-7].variable), *(yyvsp[-5].parameter_list), *tmp, *(yyvsp[-3].parameter_list), *(yyvsp[-1].program)); }
#line 1385 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 87 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing input variables"); }
#line 1391 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 88 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing input variables"); }
#line 1397 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 89 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing inout variables"); }
#line 1403 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 90 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing inout variables"); }
#line 1409 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 91 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing out variables"); }
#line 1415 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 92 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing out variables"); }
#line 1421 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 93 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing out variables"); }
#line 1427 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 95 "syntax.y" /* yacc.c:1646  */
    { (yyval.parameter) = new param(*(yyvsp[-2].variable),(yyvsp[0].value_type)); }
#line 1433 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 96 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing variable declaration"); }
#line 1439 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 97 "syntax.y" /* yacc.c:1646  */
    { handleError("Error: missing \':\' in this declaration"); }
#line 1445 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 99 "syntax.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = new std::map<string,type*>(); (yyval.parameter_list)->insert(*(yyvsp[0].parameter)); }
#line 1451 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 100 "syntax.y" /* yacc.c:1646  */
    { (yyval.parameter_list) = (yyvsp[-2].parameter_list); (yyval.parameter_list)->insert(*(yyvsp[0].parameter)); }
#line 1457 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 101 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing parameters"); }
#line 1463 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 103 "syntax.y" /* yacc.c:1646  */
    { (yyval.value_type) = new type("matrix",*(yyvsp[-1].attribute_list)); }
#line 1469 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 104 "syntax.y" /* yacc.c:1646  */
    { (yyval.value_type) = new type("vector",*(yyvsp[-1].attribute_list)); }
#line 1475 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 105 "syntax.y" /* yacc.c:1646  */
    { (yyval.value_type) = new type("tensor",*(yyvsp[-1].attribute_list)); }
#line 1481 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 106 "syntax.y" /* yacc.c:1646  */
    { (yyval.value_type) = new type(scalar); }
#line 1487 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 107 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing matrix attributes"); }
#line 1493 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 108 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing vector attributes");}
#line 1499 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 109 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing vector attributes");}
#line 1505 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 111 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("orientation","row"); }
#line 1511 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 112 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("orientation","row"); }
#line 1517 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 113 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("orientation","column"); }
#line 1523 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 114 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("container", *(yyvsp[0].variable)); }
#line 1529 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 115 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("orientation","column"); }
#line 1535 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 116 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("format","general"); }
#line 1541 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 117 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("format","triangular"); }
#line 1547 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 118 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("format","general"); }
#line 1553 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 119 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("format","triangular"); }
#line 1559 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 120 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("uplo","upper"); }
#line 1565 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 121 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("uplo","lower"); }
#line 1571 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 122 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("uplo","upper"); }
#line 1577 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 123 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("uplo","lower"); }
#line 1583 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 124 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("diag","unit"); }
#line 1589 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 125 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("diag","nonunit"); }
#line 1595 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 126 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("diag","unit"); }
#line 1601 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 127 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute) = new attrib("diag","nonunit"); }
#line 1607 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 129 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute_list) = new std::map<string,string>(); (yyval.attribute_list)->insert(*(yyvsp[0].attribute)); }
#line 1613 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 130 "syntax.y" /* yacc.c:1646  */
    { (yyval.attribute_list) = (yyvsp[-2].attribute_list); (yyval.attribute_list)->insert(*(yyvsp[0].attribute)); }
#line 1619 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 132 "syntax.y" /* yacc.c:1646  */
    { (yyval.program) = new std::vector<stmt*>(); }
#line 1625 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 133 "syntax.y" /* yacc.c:1646  */
    { (yyval.program) = (yyvsp[-1].program); (yyval.program)->push_back((yyvsp[0].statement)); }
#line 1631 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 134 "syntax.y" /* yacc.c:1646  */
    {handleError("Error in program body"); }
#line 1637 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 136 "syntax.y" /* yacc.c:1646  */
    { (yyval.statement) = new stmt((yyvsp[-2].variable),(yyvsp[0].expression)); }
#line 1643 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 137 "syntax.y" /* yacc.c:1646  */
    { handleError("Error parsing operations"); }
#line 1649 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 139 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new scalar_in((yyvsp[0].number)); }
#line 1655 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 140 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new variable((yyvsp[0].variable)); }
#line 1661 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 141 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(add, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 1667 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 142 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(subtract, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 1673 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 143 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(multiply, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 1679 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 144 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(negate_op, (yyvsp[0].expression)); }
#line 1685 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 145 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(trans, (yyvsp[-1].expression)); }
#line 1691 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 146 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = (yyvsp[-1].expression); }
#line 1697 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 147 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(squareroot, (yyvsp[-1].expression)); }
#line 1703 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 148 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(divide, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 1709 "syntax.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 149 "syntax.y" /* yacc.c:1646  */
    { (yyval.expression) = new operation(squareroot, new operation(multiply, new operation(trans, (yyvsp[-1].expression)), (yyvsp[-1].expression))); }
#line 1715 "syntax.tab.c" /* yacc.c:1646  */
    break;


#line 1719 "syntax.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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

  /* Else will try to reuse lookahead token after shifting the error
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 151 "syntax.y" /* yacc.c:1906  */

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
