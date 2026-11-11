%{
#include <stdio.h>
#include <ctype.h>

int yylex();
void yyerror(char *s);
%}

%token LETTER DIGIT

%%

start
    : variable '\n'
      {
          printf("Valid Variable\n");
      }
    ;

variable
    : LETTER tail
    ;

tail
    : LETTER tail
    | DIGIT tail
    |
    ;

%%

int yylex()
{
    int c;

    c = getchar();

    if (isalpha(c))
        return LETTER;

    if (isdigit(c))
        return DIGIT;

    return c;
}

void yyerror(char *s)
{
    printf("Invalid Variable\n");
}

int main()
{
    printf("Enter variable: ");
    yyparse();
    return 0;
}
