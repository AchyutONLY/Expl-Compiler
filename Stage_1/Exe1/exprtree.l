%{
	#include <stdio.h>
	#include <stdlib.h>
	#include "y.tab.h"
	void yyerror(const char *);
	#include "exprtree.h"
%}

%%

[0-9]+ {yylval.no=makeLeafNode(atoi(yytext)); return NUM;}
[-+*/] {return yytext[0];}
[\n] {return END;}
. {yyerror("Invalid Character\n"); exit(1);}

%%

int yywrap()
{
	return 1;
}
