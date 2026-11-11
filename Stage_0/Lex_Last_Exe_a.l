%{
	#include<stdio.h>
	int count_char;
	int count_word;
	int count_lines;
%}

%%
\n {count_lines++;}
[ \t]+ {count_char+=yyleng;}
[^ \t\n]+ {count_word++;count_char+=yyleng;}
%%

int yywrap()
{
	return 1;
}

int main()
{
	printf("Enter the input data:");
	yylex();
	printf("Number of characters:%d\n",count_char);
	printf("Number of words:%d\n",count_word);
	printf("Number of lines:%d\n",count_lines);
	
	return 0;
}


