%{
#include <stdio.h>
#include <stdlib.h>
#include "exprtree.h"
#include "exprtree.c"

int yylex(void);
void yyerror(const char *s);
extern FILE *yyin;

int currentType;
%}

%union {
    char *str;
}

%token DECL ENDDECL INT STR
%token <str> ID

%type <str> VarList

%%

program:
      DECL DeclList ENDDECL
        {
            PrintGsymbolTable();
        }
    | DECL ENDDECL
        {
            PrintGsymbolTable();
        }
    ;

DeclList:
      DeclList Decl
    | Decl
    ;

Decl:
      Type VarList ';'
    ;

Type:
      INT
        {
            currentType = TYPE_INT;
        }
    | STR
        {
            currentType = TYPE_STR;
        }
    ;

VarList:
      VarList ',' ID
        {
            Install($3, currentType, 1);
            free($3);
        }
    | ID
        {
            Install($1, currentType, 1);
            free($1);
        }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "Syntax error: %s\n", s);
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (yyin == NULL)
    {
        perror(argv[1]);
        return 1;
    }

    yyparse();

    fclose(yyin);
    return 0;
}