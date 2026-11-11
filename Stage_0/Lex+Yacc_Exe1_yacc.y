%{
      #include <stdio.h>
      #include <stdlib.h>
      int yylex();
      void yyerror(const char *s);
%}
%left '+'
%left '*'

%union
{
	char character;
}

%token <character>ALPHABET 
%token NEWLINE



%%

start : expr NEWLINE  {
                        printf("\nComplete\n");
                        exit(0);
                      }
  ;

expr:  expr '+' expr        {printf("+ ");}
  | expr '*' expr     {printf("* ");}
  | '(' expr ')'
  | ALPHABET             {printf("%c ",$<character>1);}
  ;

%%

void yyerror(const char *s)
{
    printf("yyerror  %s\n",s);
}
int main()
{
  yyparse();
  return 1;
}
