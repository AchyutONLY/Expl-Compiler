%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "y.tab.h"
    #include "exprtree.h"
    int number;
    char *variable;
    void yyerror(char const *);
%}

%%

[0-9]+  {
            number=atoi(yytext);
            yylval.no=createTree(0,number,NODE_NUM,NULL,NULL,NULL);
            return NUM;
        }

[a-z]   {
            variable=(char *)malloc(strlen(yytext)+1);
            strcpy(variable,yytext);
            yylval.no=createTree(1,0,NODE_ID,variable,NULL,NULL);
            return ID;
        }

"begin" {return T_BEGIN;}
"end" {return T_END;}
"read" {return READ;}
"write" {return WRITE;}
"+" {return PLUS;}
"-" {return MINUS;}
"*" {return MUL;}
"/" {return DIV;}
"=" {return ASSGN;}
[ \t\n] {}
[();] {return *yytext;}
.   {
        yyerror("unknown character\n");
        exit(1);
    }

%%

int yywrap()
{
    return 1;
}