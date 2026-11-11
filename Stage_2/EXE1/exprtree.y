%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "exprtree.h"
    #include "exprtree.c"
    int yylex(void);
    extern FILE *yyin;
    FILE *fp;
    void yyerror(char const *);
    int variable[26];
    int evaluate(struct tnode *t);
%}

%union
{
    struct tnode *no;
}

%token T_BEGIN T_END READ WRITE PLUS MINUS MUL DIV ASSGN
%token <no> NUM ID
%left PLUS MINUS
%left MUL DIV

%type <no> program Slist Stmt InputStmt OutputStmt AsgStmt expr

%%

program:T_BEGIN Slist T_END ';' {
                                    $$=$2;
                                    evaluate($2);
                                }
        | T_BEGIN T_END ';'     {
                                    $$=NULL;
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

int evaluate(struct tnode *t)
{
    if(t==NULL)
    {
        return 0;
    }
    switch(t->nodetype)
    {
        case NODE_NUM:
        {
            return t->val;
        }
        case NODE_ID:
        {
            int index=t->varname[0]-'a';
            return variable[index];
        }
        case NODE_PLUS:
        {
            return evaluate(t->left)+evaluate(t->right);
        }
        case NODE_MINUS:
        {
            return evaluate(t->left)-evaluate(t->right);
        }
        case NODE_MUL:
        {
            return evaluate(t->left)*evaluate(t->right);
        }
        case NODE_DIV:
        {
            return evaluate(t->left)/evaluate(t->right);
        }
        case NODE_ASSGN:
        {
            int index=t->left->varname[0]-'a';
            variable[index]=evaluate(t->right);
            return variable[index];
        }
        case NODE_READ:
        {
            int index=t->left->varname[0]-'a';
            printf("Enter the value of variable %c: ",t->left->varname[0]);
            scanf("%d",&variable[index]);
            return variable[index];
        }
        case NODE_WRITE:
        {
            printf("%d\n",evaluate(t->left));
            return 0;
        }
        case NODE_CONNECTOR:
        {
            evaluate(t->left);
            evaluate(t->right);
            return 0;
        }
    }
}

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
    fclose(fp);
    return 0;
}