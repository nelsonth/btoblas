%{
#include <math.h>
#include <string>
#include "syntax.hpp"
#include "syntax.tab.hpp"

extern int bto_yy_input(char *b, int maxBuffer);
    extern void BeginToken(char *t);
    
#define YY_INPUT(buf,result,max_size)  {\
    result = bto_yy_input(buf,max_size); \
    if (  result <= 0  ) \
    result = YY_NULL; \
    }
    
%}

DIGIT     [0-9]
VAR       [a-zA-Z][a-zA-Z0-9]*

%option yylineno

%%

{DIGIT}+            { BeginToken(yytext); 
                      yylval.number = atoi(yytext); 
                      return (NUM); }
{DIGIT}+'.'{DIGIT}* { BeginToken(yytext); 
                      yylval.number = atof(yytext); 
                      return (NUM); }

"in" { BeginToken(yytext); return (IN); }
"inout" { BeginToken(yytext); return (INOUT); }
"out" { BeginToken(yytext); return (OUT); }
"row" { BeginToken(yytext); return (ROW); }
"column" { BeginToken(yytext); return (COLUMN); }
"vector" { BeginToken(yytext); return (VECTOR); }
"matrix" { BeginToken(yytext); return (MATRIX); }
"tensor" { BeginToken(yytext); return (TENSOR); }
"scalar" { BeginToken(yytext); return (SCALAR); }
"orientation" { BeginToken(yytext); return (ORIENTATION); }
"container" { BeginToken(yytext); return (CONTAINER); }
"format" { BeginToken(yytext); return (FORMAT); }
"general" { BeginToken(yytext); return (GENERAL); }
"triangular" { BeginToken(yytext); return (TRIANGULAR); }
"uplo" { BeginToken(yytext); return (UPLO); }
"upper" { BeginToken(yytext); return (UPPER); }
"lower" { BeginToken(yytext); return (LOWER); }
"diag" { BeginToken(yytext); return (DIAG); }
"unit" { BeginToken(yytext); return (UNIT); }
"nonunit" { BeginToken(yytext); return (NONUNIT); }
"squareroot" { BeginToken(yytext); return (SQUAREROOT); }

{VAR}               { BeginToken(yytext); 
                      yylval.variable = new std::string(yytext); 
                      return(VAR); }

[':=+\-(){}*,/|]             { BeginToken(yytext); return *yytext; }


"{"[^}\n]*"}"       { /* eat up one-line comments */
                      BeginToken(yytext);  }

[ \t\n]+            { /* eat up whitespace */
                      BeginToken(yytext);   }

.                    printf( "Unrecognized character: %s\n", yytext );

%%
int yywrap() { return 1; }
