%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"
#include "exprtree.h"
int number;
char *variableeee;
void yyerror(const char *);
%}
%%
[0-9]+ {
    number = atoi(yytext);
    yylval.no = createTree(number, TYPE_INT, NULL, NODE_NUM, NULL, NULL, NULL);
    return NUM;
}
"read"      { return READ; }
"write"     { return WRITE; }
"if"        { return IF; }
"then"      { return THEN; }
"else"      { return ELSE; }
"endif"     { return ENDIF; }
"do"        { return DO; }
"while"     { return WHILE; }
"endwhile"  { return ENDWHILE; }
"break"     { return BREAK; }
"continue"  { return CONTINUE; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MUL; }
"/"         { return DIV; }
"%"         { return MOD; }
"<="        { return LE; }
">="        { return GE; }
"=="        { return EQ; }
"!="        { return NE; }
"<"         { return LT; }
">"         { return GT; }
"="         { return ASSGN; }
[a-z]+ {
    variableeee = strdup(yytext);
    yylval.no = createTree(0, TYPE_INT, variableeee, NODE_ID, NULL, NULL, NULL);
    free(variableeee);
    return ID;
}
[ \t\n]+ {}
[();]    { return *yytext; }
.        { yyerror("unknown character"); exit(1); }
%%
int yywrap(void) {
    return 1;
}