%{
	#include <stdio.h>
	#include <stdlib.h>
%}

%%

//[^\n]* {printf("%s\n",yytext);}

/\*[.|\n]*\*/ {printf("%s\n",yytext);}

%%

int yywrap()
{
	return 1;
}

int main()
{
	FILE *fp=fopen("temp_comments.txt","r");
	yyin=fp;
	yylex();
	return 0;
}
