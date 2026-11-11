%{
	#include <stdlib.h>
	#include <stdio.h>
	#include "y.tab.h"
	#include "exprtree.h"
	void yyerror(const char *);
	int number;
%}

%%

[0-9]+	{
		number=atoi(yytext);
		yylval.no=makeLeafNode(number);
		return NUM;
	}
[\t] {}
[+-/*()] {return *yytext;}
[\n] {return END;}
.	{
		yyerror("Invalid Character\n");
		exit(1);
	}

%%

int yywrap(void)
{
	return 1;
}
