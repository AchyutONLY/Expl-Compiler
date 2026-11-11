%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exprtree.h"
#include "exprtree.c"

int yylex(void);
void yyerror(const char *);
extern FILE *yyin;

FILE *fp, *target;
int currentDeclType;
%}

%union
{
    struct tnode *no;
    char *str;
    int type;
}

%token <no> NUM STRCONST
%token <str> ID
%token DECL ENDDECL INT STR
%token READ WRITE
%token IF THEN ELSE ENDIF
%token DO WHILE ENDWHILE
%token BREAK CONTINUE
%token LT GT LE GE EQ NE
%token MUL DIV MOD PLUS MINUS ASSGN AMP

%type <no> E Slist Stmt LValue
%type <type> Type PointerStars

%left EQ NE LT GT LE GE
%left PLUS MINUS
%left MUL DIV MOD
%right AMP USTAR

%%

program:
      Declarations Slist
      {
          PrintGsymbolTable();
          codeGen($2, target);
      }
      ;

Declarations:
      DECL DeclList ENDDECL
    | DECL ENDDECL
    ;

DeclList:
      DeclList Decl
    | Decl
    ;

Decl:
      Type
      {
          currentDeclType = $1;
      }
      VarList ';'
      ;

Type:
      INT { $$ = TYPE_INT; }
    | STR { $$ = TYPE_STR; }
    ;

VarList:
      VarList ',' ID
      {
          Install($3, currentDeclType, 1, 0, 0, 1, 1, 0);
          free($3);
      }
    | VarList ',' PointerStars ID
      {
          Install($4, currentDeclType, 1, 0, 0, 1, 1, $3);
          free($4);
      }
    | VarList ',' ID '[' NUM ']'
      {
          int n = $5->val;
          if (n <= 0) { fprintf(stderr, "Error: array '%s' must have positive size\n", $3); exit(1); }
          Install($3, currentDeclType, n, 1, 1, n, 1, 0);
          free($3); free($5);
      }
    | VarList ',' ID '[' NUM ']' '[' NUM ']'
      {
          int rows = $5->val, cols = $8->val;
          if (rows <= 0 || cols <= 0) { fprintf(stderr, "Error: array '%s' dimensions must be positive\n", $3); exit(1); }
          Install($3, currentDeclType, rows * cols, 1, 2, rows, cols, 0);
          free($3); free($5); free($8);
      }
    | ID
      {
          Install($1, currentDeclType, 1, 0, 0, 1, 1, 0);
          free($1);
      }
    | PointerStars ID
      {
          Install($2, currentDeclType, 1, 0, 0, 1, 1, $1);
          free($2);
      }
    | ID '[' NUM ']'
      {
          int n = $3->val;
          if (n <= 0) { fprintf(stderr, "Error: array '%s' must have positive size\n", $1); exit(1); }
          Install($1, currentDeclType, n, 1, 1, n, 1, 0);
          free($1); free($3);
      }
    | ID '[' NUM ']' '[' NUM ']'
      {
          int rows = $3->val, cols = $6->val;
          if (rows <= 0 || cols <= 0) { fprintf(stderr, "Error: array '%s' dimensions must be positive\n", $1); exit(1); }
          Install($1, currentDeclType, rows * cols, 1, 2, rows, cols, 0);
          free($1); free($3); free($6);
      }
    ;

PointerStars:
      MUL { $$ = 1; }
    | PointerStars MUL { $$ = $1 + 1; }
    ;

Slist:
      Slist Stmt
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_CONNECTOR, NULL, $1, $2, NULL);
      }
    | Stmt { $$ = $1; }
    ;

/* L-value can be an ordinary variable, 1D/2D array element, or *pointer. */
LValue:
      ID
      {
          struct Gsymbol *g = Lookup($1);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $1); exit(1); }
          if (g->isArray) { fprintf(stderr, "Error: array '%s' must be indexed\n", $1); exit(1); }
          $$ = createTree(0, g->type, $1, NULL, NODE_ID, g, NULL, NULL, NULL);
          free($1);
      }
    | ID '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $1); exit(1); }
          if (!g->isArray || g->dims != 1) { fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $1); exit(1); }
          if ($3->type != TYPE_INT) { fprintf(stderr, "Error: array index must be an integer\n"); exit(1); }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY, g, $3, NULL, NULL);
          free($1);
      }
    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $1); exit(1); }
          if (!g->isArray || g->dims != 2) { fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $1); exit(1); }
          if ($3->type != TYPE_INT || $6->type != TYPE_INT) { fprintf(stderr, "Error: array index must be an integer\n"); exit(1); }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY, g, $3, $6, NULL);
          free($1);
      }
    | MUL E %prec USTAR
      {
          if (!isPointerType($2->type))
          {
              fprintf(stderr, "Error: dereference requires a pointer operand\n");
              exit(1);
          }
          $$ = createTree(0, makePtrType(baseTypeOf($2->type), pointerDepth($2->type)-1), NULL, NULL,
                          NODE_DEREF, NULL, $2, NULL, NULL);
      }
    ;

Stmt:
      LValue ASSGN E ';'
      {
          if ($1->nodetype == NODE_ID && $1->Gentry->isArray)
          {
              fprintf(stderr, "Error: array must be indexed\n"); exit(1);
          }
          if ($1->type != $3->type)
          {
              fprintf(stderr, "Type Mismatch in assignment\n"); exit(1);
          }
          $$ = createTree(0, $1->type, NULL, NULL, NODE_ASSGN, NULL, $1, $3, NULL);
      }
    | READ '(' LValue ')' ';'
      {
          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Type Mismatch: read expects an integer lvalue\n"); exit(1);
          }
          $$ = createTree(0, -1, NULL, NULL, NODE_READ, NULL, $3, NULL, NULL);
      }
    | WRITE '(' E ')' ';'
      {
          if ($3->type != TYPE_INT && $3->type != TYPE_STR)
          {
              fprintf(stderr, "Type Mismatch in write\n"); exit(1);
          }
          $$ = createTree(0, -1, NULL, NULL, NODE_WRITE, NULL, $3, NULL, NULL);
      }
    | IF '(' E ')' THEN Slist ENDIF ';'
      { $$ = createTree(0, -1, NULL, NULL, NODE_IF, NULL, $3, $6, NULL); }
    | IF '(' E ')' THEN Slist ELSE Slist ENDIF ';'
      { $$ = createTree(0, -1, NULL, NULL, NODE_IF, NULL, $3, $6, $8); }
    | WHILE '(' E ')' DO Slist ENDWHILE ';'
      { $$ = createTree(0, -1, NULL, NULL, NODE_WHILE, NULL, $3, $6, NULL); }
    | BREAK ';'
      { $$ = createTree(0, -1, NULL, NULL, NODE_BREAK, NULL, NULL, NULL, NULL); }
    | CONTINUE ';'
      { $$ = createTree(0, -1, NULL, NULL, NODE_CONTINUE, NULL, NULL, NULL, NULL); }
    ;

E:
      E PLUS E { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_PLUS, NULL, $1, $3, NULL); }
    | E MINUS E { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MINUS, NULL, $1, $3, NULL); }
    | E MUL E { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MUL, NULL, $1, $3, NULL); }
    | E DIV E { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_DIV, NULL, $1, $3, NULL); }
    | E MOD E { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MOD, NULL, $1, $3, NULL); }
    | E LT E { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_LT, NULL, $1, $3, NULL); }
    | E GT E { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_GT, NULL, $1, $3, NULL); }
    | E LE E { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_LE, NULL, $1, $3, NULL); }
    | E GE E { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_GE, NULL, $1, $3, NULL); }
    | E EQ E { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_EQ, NULL, $1, $3, NULL); }
    | E NE E { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_NE, NULL, $1, $3, NULL); }
    | AMP ID %prec AMP
      {
          struct Gsymbol *g = Lookup($2);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $2); exit(1); }
          if (g->isArray) { fprintf(stderr, "Error: use an indexed array element with &\n"); exit(1); }
          $$ = createTree(0, makePtrType(g->baseType, g->ptrDepth + 1), NULL, NULL,
                          NODE_ADDR, g, NULL, NULL, NULL);
          free($2);
      }
    | AMP ID '[' E ']' %prec AMP
      {
          struct Gsymbol *g = Lookup($2);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $2); exit(1); }
          if (!g->isArray || g->dims != 1) { fprintf(stderr, "Error: invalid 1D array reference in &\n"); exit(1); }
          if ($4->type != TYPE_INT) { fprintf(stderr, "Error: array index must be an integer\n"); exit(1); }
          struct tnode *arr = createTree(0, g->type, $2, NULL, NODE_ARRAY, g, $4, NULL, NULL);
          $$ = createTree(0, makePtrType(g->baseType, 1), NULL, NULL, NODE_ADDR, NULL, arr, NULL, NULL);
          free($2);
      }
    | AMP ID '[' E ']' '[' E ']' %prec AMP
      {
          struct Gsymbol *g = Lookup($2);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $2); exit(1); }
          if (!g->isArray || g->dims != 2) { fprintf(stderr, "Error: invalid 2D array reference in &\n"); exit(1); }
          if ($4->type != TYPE_INT || $7->type != TYPE_INT) { fprintf(stderr, "Error: array index must be an integer\n"); exit(1); }
          struct tnode *arr = createTree(0, g->type, $2, NULL, NODE_ARRAY, g, $4, $7, NULL);
          $$ = createTree(0, makePtrType(g->baseType, 1), NULL, NULL, NODE_ADDR, NULL, arr, NULL, NULL);
          free($2);
      }
    | MUL E %prec USTAR
      {
          if (!isPointerType($2->type)) { fprintf(stderr, "Error: dereference requires a pointer operand\n"); exit(1); }
          $$ = createTree(0, makePtrType(baseTypeOf($2->type), pointerDepth($2->type)-1), NULL, NULL,
                          NODE_DEREF, NULL, $2, NULL, NULL);
      }
    | '(' E ')' { $$ = $2; }
    | NUM { $$ = $1; }
    | STRCONST { $$ = $1; }
    | ID
      {
          struct Gsymbol *g = Lookup($1);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $1); exit(1); }
          if (g->isArray) { fprintf(stderr, "Error: array '%s' must be indexed\n", $1); exit(1); }
          $$ = createTree(0, g->type, $1, NULL, NODE_ID, g, NULL, NULL, NULL);
          free($1);
      }
    | ID '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $1); exit(1); }
          if (!g->isArray || g->dims != 1) { fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $1); exit(1); }
          if ($3->type != TYPE_INT) { fprintf(stderr, "Error: array index must be an integer\n"); exit(1); }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY, g, $3, NULL, NULL);
          free($1);
      }
    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);
          if (g == NULL) { fprintf(stderr, "Error: variable '%s' is not declared\n", $1); exit(1); }
          if (!g->isArray || g->dims != 2) { fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $1); exit(1); }
          if ($3->type != TYPE_INT || $6->type != TYPE_INT) { fprintf(stderr, "Error: array index must be an integer\n"); exit(1); }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY, g, $3, $6, NULL);
          free($1);
      }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "error: %s\n", s);
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 2) { printf("Usage: %s <input.expl>\n", argv[0]); return 1; }

    fp = fopen(argv[1], "r");
    if (!fp) { perror(argv[1]); return 1; }

    target = fopen("target.xsm", "w");
    if (!target) { perror("target.xsm"); fclose(fp); return 1; }

    fprintf(target, "0\n2056\n0\n0\n0\n0\n0\n0\n");
    yyin = fp;

    if (yyparse() != 0)
    {
        fclose(fp); fclose(target); return 1;
    }

    emit_exit(target);
    fclose(fp);
    fclose(target);
    return 0;
}