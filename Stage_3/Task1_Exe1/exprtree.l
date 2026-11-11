%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "y.tab.h"
    #include "exprtree.h"
    int number;
    char *variableeee;
    void yyerror(char const *);
%}

%%

[0-9]+  {
            number=atoi(yytext);
            yylval.no=createTree(number,TYPE_INT,NULL,NODE_NUM,NULL,NULL,NULL);
            return NUM;
        }

"read" {return READ;}
"write" {return WRITE;}
"+" {return PLUS;}
"-" {return MINUS;}
"*" {return MUL;}
"/" {return DIV;}

"%" {return MOD;}
"if" {return IF;}
"then" {return THEN;}
"else" {return ELSE;}
"endif" {return ENDIF;}
"do" {return DO;}
"while" {return WHILE;}
"endwhile" {return ENDWHILE;}

[a-z]   {
            variableeee=(char *)malloc(strlen(yytext)+1);
            strcpy(variableeee,yytext);
            yylval.no=createTree(0,TYPE_INT,variableeee,NODE_ID,NULL,NULL,NULL);
            return ID;
        }

"<=" {return LE;}
">=" {return GE;}
"==" {return EQ;}
"!=" {return NE;}
"<" {return LT;}
">" {return GT;}
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