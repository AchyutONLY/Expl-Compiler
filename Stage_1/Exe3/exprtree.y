%{
	#include <stdlib.h>
	#include <stdio.h>
	#include "exprtree.h"
	#include "exprtree.c"
	#include "codeGen.c"
	int yylex(void);
	extern FILE *yyin;
	FILE *fp;
	FILE *target;
	void print(int);
	void yyerror(const char *);
%}

%union
{
	struct tnode *no;
}
%type <no> expr program NUM END
%token NUM END
%left '+' '-'
%left '*' '/'

%%

program: expr END	{
				$$=$2;
				int r=codegen($1);
				print(r);
			}

expr: '+' expr expr 	{$$=makeOperatorNode('+',$2,$3);}
    | '-' expr expr 	{$$=makeOperatorNode('-',$2,$3);}
    | '*' expr expr 	{$$=makeOperatorNode('*',$2,$3);}
    | '/' expr expr 	{$$=makeOperatorNode('/',$2,$3);}
    | '(' expr ')' 	{$$=$2;}
    | NUM		{$$=$1;}
    ;
%%

void yyerror(char const *s)
{
	printf("yyerror: %s",s);
}

void print(int r)
{
	fprintf(target, "MOV R2, \"Write\"\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "MOV R2, -1\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "PUSH R%d\n", r);
	fprintf(target, "PUSH R2\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "CALL 0\n");
	fprintf(target, "POP R0\n");
	fprintf(target, "POP R1\n");
	fprintf(target, "POP R1\n");
	fprintf(target, "POP R1\n");
	fprintf(target, "POP R1\n");
	fprintf(target, "MOV R2, \"Exit\"\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "PUSH R2\n");
	fprintf(target, "CALL 0\n");
	exit(1);
}

int main(int argc,char *argv[])
{
	if(argc<2)
	{
		printf("You must give input filename as argument.\n");
		exit(1);
	}
	else
	{
		target=fopen("target.xsm","w");
		fprintf(target,"0\n2056\n0\n0\n0\n0\n0\n0\n");
		fp=fopen(argv[1],"r");
		if(!fp)
		{
			printf("Something wring with the input provided.\n");
			exit(1);
		}
		else
		{
			yyin=fp;
		}
	}
	yyparse();
	return 0;
}
