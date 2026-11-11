%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "exprtree.h"
    #include "exprtree.c"
    #include "codegen.c"
    #include "initialize.c"
    int yylex(void);
    extern FILE *yyin;
    FILE *fp;
    FILE *target;
    void print(int);
    void yyerror(char const *);
%}

%union
{
    struct tnode *no;
}

%token T_BEGIN READ WRITE PLUS MINUS MUL DIV ASSGN
%token <no> NUM ID T_END
%left PLUS MINUS
%left MUL DIV

%type <no> program Slist Stmt InputStmt OutputStmt AsgStmt expr

%%

program:T_BEGIN Slist T_END ';'     {
                                    $$=$3;
                                    initialize();
                                    codegen($2);
                                    fprintf(target,"MOV R0, \"Exit\"\n");
                                    fprintf(target,"PUSH R0\n");
                                    fprintf(target,"PUSH R1\n");
                                    fprintf(target,"PUSH R1\n");
                                    fprintf(target,"PUSH R1\n");
                                    fprintf(target,"PUSH R1\n");
                                    fprintf(target,"CALL 0\n");
                                    fclose(target);
                                }
      | T_BEGIN T_END ';'   {
                            $$=$2;
                        }

Slist: Slist Stmt   {$$=createTree(1,0,NODE_CONNECTOR,NULL,$1,$2);}
    | Stmt  {$$=$1;}

Stmt: InputStmt     {$$=$1;}
    | OutputStmt    {$$=$1;}
    | AsgStmt   {$$=$1;}

InputStmt: READ '(' ID ')' ';'  {$$=createTree(1,0,NODE_READ,NULL,$3,NULL);}

OutputStmt: WRITE '(' expr ')' ';'  {$$=createTree(1,0,NODE_WRITE,NULL,$3,NULL);}

AsgStmt: ID ASSGN expr ';' {$$=createTree(1,0,NODE_ASSGN,NULL,$1,$3);}

expr: expr PLUS expr    {$$=createTree(1,0,NODE_PLUS,NULL,$1,$3);}
    | expr MINUS expr   {$$=createTree(1,0,NODE_MINUS,NULL,$1,$3);}
    | expr MUL expr     {$$=createTree(1,0,NODE_MUL,NULL,$1,$3);}
    | expr DIV expr     {$$=createTree(1,0,NODE_DIV,NULL,$1,$3);}
    | '(' expr ')'  {$$=$2;}
    | NUM   {$$=$1;}
    | ID    {$$=$1;}

%%

void yyerror(char const *s)
{
    printf("yyerror: %s\n",s);
}

int main(int argc,char *argv[])
{
    if(argc<2)
    {
        printf("Please provide an input filename\n");
        exit(1);
    }
    else
    {
        fp=fopen(argv[1],"r");
        if(!fp)
        {
            printf("Invalid input file\n");
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