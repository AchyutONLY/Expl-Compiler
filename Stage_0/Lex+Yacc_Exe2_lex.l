%{
    #include <stdio.h>
   #include "y.tab.h"
%}

%%

[a-z]+ {
          yylval.character = yytext;
          return ALPHABET;
        }
"+"   return yytext[0];
"*"   return yytext[0];
[()]   return yytext[0];
[\n]      return NEWLINE;

%%

int yywrap()
{
 return 1;
}

