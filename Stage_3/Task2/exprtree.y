%{
#include <stdio.h>
#include <stdlib.h>
#include "exprtree.h"
#include "exprtree.c"

int yylex(void);
void yyerror(const char*);
extern FILE* yyin;
FILE* fp, *target;
%}

%union {
    struct tnode* no;
}

%token <no> NUM ID
%type <no> E Slist Stmt

%token IF THEN ELSE ENDIF DO WHILE ENDWHILE READ WRITE LT GT LE GE EQ NE MUL DIV MOD PLUS MINUS ASSGN

%left EQ NE LT GT LE GE
%left PLUS MINUS
%left MUL DIV MOD

%%

program:
    Slist { codeGen($1, target); }
    ;

Slist:
    Slist Stmt { $$ = createTree(0, -1, NULL, NODE_CONNECTOR, $1, $2, NULL); }
    | Stmt { $$ = $1; }
    ;

Stmt:
    ID ASSGN E ';' { $$ = createTree(0, TYPE_INT, NULL, NODE_ASSGN, $1, $3, NULL); }
    | READ '(' ID ')' ';' { $$ = createTree(0, -1, NULL, NODE_READ, $3, NULL, NULL); }
    | WRITE '(' E ')' ';' { $$ = createTree(0, -1, NULL, NODE_WRITE, $3, NULL, NULL); }
    | IF '(' E ')' THEN Slist ENDIF ';' { $$ = createTree(0, -1, NULL, NODE_IF, $3, $6, NULL); }
    | IF '(' E ')' THEN Slist ELSE Slist ENDIF ';' { $$ = createTree(0, -1, NULL, NODE_IF, $3, $6, $8); }
    | WHILE '(' E ')' DO Slist ENDWHILE ';' { $$ = createTree(0, -1, NULL, NODE_WHILE, $3, $6, NULL); }
    ;

E:
    E PLUS E { $$ = createTree(0, TYPE_INT, NULL, NODE_PLUS, $1, $3, NULL); }
    | E MINUS E { $$ = createTree(0, TYPE_INT, NULL, NODE_MINUS, $1, $3, NULL); }
    | E MUL E { $$ = createTree(0, TYPE_INT, NULL, NODE_MUL, $1, $3, NULL); }
    | E DIV E { $$ = createTree(0, TYPE_INT, NULL, NODE_DIV, $1, $3, NULL); }
    | E MOD E { $$ = createTree(0, TYPE_INT, NULL, NODE_MOD, $1, $3, NULL); }
    | E LT E { $$ = createTree(0, TYPE_BOOL, NULL, NODE_LT, $1, $3, NULL); }
    | E GT E { $$ = createTree(0, TYPE_BOOL, NULL, NODE_GT, $1, $3, NULL); }
    | E LE E { $$ = createTree(0, TYPE_BOOL, NULL, NODE_LE, $1, $3, NULL); }
    | E GE E { $$ = createTree(0, TYPE_BOOL, NULL, NODE_GE, $1, $3, NULL); }
    | E EQ E { $$ = createTree(0, TYPE_BOOL, NULL, NODE_EQ, $1, $3, NULL); }
    | E NE E { $$ = createTree(0, TYPE_BOOL, NULL, NODE_NE, $1, $3, NULL); }
    | '(' E ')' { $$ = $2; }
    | NUM { $$ = $1; }
    | ID { $$ = $1; }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "error: %s\n", s);
    exit(1);
}

int main(int argc, char** argv) {
    if (argc != 2)
        return 1;
    fp = fopen(argv[1], "r");
    if (!fp)
        return 1;
    target = fopen("target.xsm", "w");
    fprintf(target, "0\n2056\n0\n0\n0\n0\n0\n0\n");
    yyin = fp;
    yyparse();
    emit_exit(target);
    fclose(fp);
    fclose(target);
    return 0;
}