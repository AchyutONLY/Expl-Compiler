%{
	#include <stdio.h>
	int integer_count=0;
	int float_count=0;
%}

%%

[0-9]*\.[0-9]+ {float_count++;}
[0-9]+ {integer_count++;}
[\n] {}
%%

int yywrap()
{
	return 1;
}

int main()
{
	printf("Enter zero or more integers and zero or more floating point numbers:\n");
	yylex();
	printf("Number of integers:%d\n",integer_count);
	printf("Number of float:%d\n",float_count);
	return 0;
}
