%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "y.tab.h" 

int number;
%}

%%
"begin"     { return START; }
"end"       { return END; }
"read"      { return READ; }
"write"     { return WRITE; }
"if"        { return IF; }
"then"      { return THEN; }
"else"      { return ELSE; }
"endif"     { return ENDIF; }
"while"     { return WHILE; }
"do"        { return DO; }
"endwhile"  { return ENDWHILE; }
"break"     { return BREAK; }
"continue"  { return CONTINUE; }
"decl"      { return DECL; }
"enddecl"   { return ENDDECL; }
"int"       { return INT; }
"str"       { return STR; }
"main"      { return MAIN; }
"return"    { return RETURN; }
"type"      { return TYPE; }
"endtype"   { return ENDTYPE; }
"class"     { return CLASS; }
"endclass"  { return ENDCLASS; }
"extends"   { return EXTENDS; }
"self"      { return SELF; }
"new"       { return NEW; }
"alloc"     { return ALLOC; }
"free"      { return FREE; }
"initialize" { return INITIALIZE; }
"NULL"      { return NULLTOK; }
"null"      { return NULLTOK; }

"<="        { return LE; }
">="        { return GE; }
"=="        { return EQ; }
"!="        { return NE; }
"<"         { return LT; }
">"         { return GT; }

[0-9]+      { 
              number = atoi(yytext); 
              yylval.no = createTree(number, inttype, NULL, NODE_NUM, NULL, NULL, NULL, NULL); 
              return NUM; 
            }

\"[^\"]*\"  { 
              yylval.no = createTree(0, strtype, yytext, NODE_STR, NULL, NULL, NULL, NULL);
              return STRING;
            }

[a-zA-Z][a-zA-Z0-9_]*   {
    yylval.no = createTree(0, inttype, yytext, NODE_ID, NULL, NULL, NULL, NULL);
    return ID;
}

[ \t\n]     { }

[=+\-*/%;()\[\]{},&.] { return *yytext; }

.           { 
              printf("Error: Unknown character '%s'\n", yytext); 
              exit(1); 
            }

%%
int yywrap() {
    return 1;
}