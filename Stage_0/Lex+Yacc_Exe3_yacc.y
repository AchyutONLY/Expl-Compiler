%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex();
void yyerror(const char *s);
%}

%union
{
    char *str;
}

%token <str> ID
%token NEWLINE

%type <str> expr

%left '+'
%left '*'

%%

start
    : expr NEWLINE
      {
          printf("%s\n", $1);
          free($1);
          exit(0);
      }
    ;

expr
    : expr '+' expr
      {
          $$ = (char *)malloc(strlen($1)+strlen($3)+4);
          sprintf($$, "+ %s %s", $1, $3);
      }
    | expr '*' expr
      {
          $$ = (char *)malloc(strlen($1)+strlen($3)+4);
          sprintf($$, "* %s %s", $1, $3);
      }
    | '(' expr ')'
      {
          $$ = $2;
      }
    | ID
      {
          $$ = $1;
      }
    ;

%%

void yyerror(const char *s)
{
    printf("Error: %s\n", s);
}

int main()
{
    yyparse();
    return 0;
}
