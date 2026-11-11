%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"
%}

%%

"decl"      { return DECL; }
"enddecl"   { return ENDDECL; }
"int"       { return INT; }
"str"       { return STR; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return ID;
}

","         { return ','; }
";"         { return ';'; }

[ \t\n\r]+  {}

. {
    fprintf(stderr, "Unknown character: %s\n", yytext);
    exit(1);
}

%%

int yywrap(void)
{
    return 1;
}