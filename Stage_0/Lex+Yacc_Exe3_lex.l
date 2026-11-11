%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "y.tab.h"
%}

%%

[a-zA-Z][a-zA-Z0-9]* {
    yylval.str = strdup(yytext);
    return ID;
}

"+"     return '+';
"*"     return '*';
"("     return '(';
")"     return ')';
\n      return NEWLINE;
[ \t]   ;

%%

int yywrap()
{
    return 1;
}
