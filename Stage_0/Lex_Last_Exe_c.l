%{
	#include <ctype.h>
	#include <stdio.h>
%}

%%

A[^ \t\n]+ {if(yyleng==3){char *str=yytext;
				for(int i=0;i<3;i++)
				{
					str[i]=toupper(str[i]);
				}
				printf("%s\n",str);}}
				
.|\n ;

%%

int yywrap()
{
	return 1;
}

int main()
{
	printf("Enter data:\n");
	yylex();
	return 0;
}
