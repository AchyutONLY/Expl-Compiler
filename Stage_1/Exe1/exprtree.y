%{
	#include <stdio.h>
	int yylex();
	void yyerror(const char *);
	#include "exprtree.h"
	void POSTFIX(struct tnode *);
	void PREFIX(struct tnode *);
%}

%union{
	struct tnode * no;
}
%token <no> NUM
%token END
%type <no> expr
%left '+' '-'
%left '*' '/'

%%

program: expr END { printf("Postfix: "); POSTFIX($1); printf("\nPrefix: "); PREFIX($1);  }

expr: expr '+' expr {$$=makeOperatorNode('+',$1,$3);}
    | expr '-' expr {$$=makeOperatorNode('-',$1,$3);}
    | expr '*' expr {$$=makeOperatorNode('*',$1,$3);}
    | expr '/' expr {$$=makeOperatorNode('/',$1,$3);}
    | '(' expr ')' {$$=$2;}
    | NUM {$$=$1;}

%%

void POSTFIX(struct tnode *t)
{
	if(t==NULL)
	{
		return;
	}
	if(t->op==NULL)
	{
		printf("%d ",t->val);
	}
	else
	{
		POSTFIX(t->left);
		POSTFIX(t->right);
		printf("%c ",*(t->op));
	}
}

void PREFIX(struct tnode *t)
{
	if(t==NULL)
	{
		return;
	}
	if(t->op==NULL)
	{
		printf("%d ",t->val);
	}
	else
	{
		printf("%c ",*(t->op));
		PREFIX(t->left);
		PREFIX(t->right);
	}
}

void yyerror(const char *s)
{
	printf("yyerror: Invalid Character\n");
}

int main()
{
	yyparse();
	return 0;
}
