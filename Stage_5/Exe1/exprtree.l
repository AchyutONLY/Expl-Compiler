%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "y.tab.h"
#include "exprtree.h"

void yyerror(const char *);
%}

%%

[0-9]+ {
    yylval.no = createTree(atoi(yytext), TYPE_INT, NULL, NULL,
                           NODE_NUM, NULL, NULL, NULL, NULL, NULL, NULL);
    return NUM;
}

\"[^\"]*\" {
    int len = strlen(yytext);
    char *s = (char *)malloc(len - 1);

    if (!s)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    strncpy(s, yytext + 1, len - 2);
    s[len - 2] = '\0';

    yylval.no = createTree(0, TYPE_STR, NULL, s,
                           NODE_STR, NULL, NULL, NULL, NULL, NULL, NULL);
    free(s);
    return STRCONST;
}

"decl"      { return DECL; }
"enddecl"   { return ENDDECL; }
"int"       { return INT; }
"str"       { return STR; }
"void"      { return VOID; }
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
"return"    { return RETURN; }
"main"      { return MAIN; }
"begin"     { return BEGINBLOCK; }
"end"       { return END; }

"AND"       { return AND; }
"OR"        { return OR; }
"NOT"       { return NOT; }
"and"       { return AND; }
"or"        { return OR; }
"not"       { return NOT; }

"<="        { return LE; }
">="        { return GE; }
"=="        { return EQ; }
"!="        { return NE; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MUL; }
"/"         { return DIV; }
"%"         { return MOD; }
"<"         { return LT; }
">"         { return GT; }
"="         { return ASSGN; }
"&"         { return AMP; }
"&&"        { return AND; }
"||"        { return OR; }
"!"         { return NOT; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);

    if (!yylval.str)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    return ID;
}

[ \t\n]+ { }

[();,\[\]{}] { return *yytext; }

. {
    yyerror("unknown character");
    exit(1);
}

%%

int yywrap(void)
{
    return 1;
}