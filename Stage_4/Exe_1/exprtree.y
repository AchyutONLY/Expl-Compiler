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

/*
 * Used while parsing a declaration:
 *
 *     int a, b, c;
 *
 * Each ID in the VarList is installed with this type.
 */
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
%token MUL DIV MOD PLUS MINUS ASSGN

%type <no> E Slist Stmt
%type <type> Type

%left EQ NE LT GT LE GE
%left PLUS MINUS
%left MUL DIV MOD

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
      {
      }
    | DECL ENDDECL
      {
      }
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
      INT
      {
          $$ = TYPE_INT;
      }
    | STR
      {
          $$ = TYPE_STR;
      }
    ;

VarList:
      VarList ',' ID
      {
          Install($3, currentDeclType, 1, 0, 0, 1, 1);
          free($3);
      }
    | VarList ',' ID '[' NUM ']'
      {
          int rows = $5->val;

          if (rows <= 0)
          {
              fprintf(stderr, "Error: array '%s' must have positive size\n", $3);
              exit(1);
          }

          Install($3, currentDeclType, rows, 1, 1, rows, 1);
          free($3);
          free($5);
      }
    | VarList ',' ID '[' NUM ']' '[' NUM ']'
      {
          int rows = $5->val;
          int cols = $8->val;

          if (rows <= 0 || cols <= 0)
          {
              fprintf(stderr, "Error: array '%s' dimensions must be positive\n", $3);
              exit(1);
          }

          Install($3, currentDeclType, rows * cols, 1, 2, rows, cols);
          free($3);
          free($5);
          free($8);
      }
    | ID
      {
          Install($1, currentDeclType, 1, 0, 0, 1, 1);
          free($1);
      }
    | ID '[' NUM ']'
      {
          int rows = $3->val;

          if (rows <= 0)
          {
              fprintf(stderr, "Error: array '%s' must have positive size\n", $1);
              exit(1);
          }

          Install($1, currentDeclType, rows, 1, 1, rows, 1);
          free($1);
          free($3);
      }
    | ID '[' NUM ']' '[' NUM ']'
      {
          int rows = $3->val;
          int cols = $6->val;

          if (rows <= 0 || cols <= 0)
          {
              fprintf(stderr, "Error: array '%s' dimensions must be positive\n", $1);
              exit(1);
          }

          Install($1, currentDeclType, rows * cols, 1, 2, rows, cols);
          free($1);
          free($3);
          free($6);
      }
    ;

Slist:
      Slist Stmt
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_CONNECTOR,
              NULL,
              $1, $2, NULL
          );
      }
    | Stmt
      {
          $$ = $1;
      }
    ;

Stmt:
      ID ASSGN E ';'
      {
          struct Gsymbol *g = Lookup($1);

          if (g == NULL)
          {
              fprintf(stderr,
                      "Error: variable '%s' is not declared\n",
                      $1);
              exit(1);
          }

          if (g->isArray)
          {
              fprintf(stderr,
                      "Error: array '%s' must be indexed\n",
                      $1);
              exit(1);
          }

          struct tnode *idnode =
              createTree(
                  0,
                  g->type,
                  $1,
                  NULL,
                  NODE_ID,
                  g,
                  NULL,
                  NULL,
                  NULL
              );

          $$ = createTree(
              0, idnode->type, NULL, NULL,
              NODE_ASSGN,
              NULL,
              idnode, $3, NULL
          );

          free($1);
      }
    | ID '[' E ']' '[' E ']' ASSGN E ';'
      {
          struct Gsymbol *g = Lookup($1);

          if (g == NULL)
          {
              fprintf(stderr, "Error: variable '%s' is not declared\n", $1);
              exit(1);
          }

          if (!g->isArray || g->dims != 2)
          {
              fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $1);
              exit(1);
          }

          if ($3->type != TYPE_INT || $6->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be an integer\n");
              exit(1);
          }

          struct tnode *arrnode =
              createTree(0, g->type, $1, NULL, NODE_ARRAY, g,
                         $3, $6, NULL);

          $$ = createTree(0, arrnode->type, NULL, NULL,
                          NODE_ASSGN, NULL, arrnode, $9, NULL);

          free($1);
      }
    | ID '[' E ']' ASSGN E ';'
      {
          struct Gsymbol *g = Lookup($1);

          if (g == NULL)
          {
              fprintf(stderr, "Error: variable '%s' is not declared\n", $1);
              exit(1);
          }

          if (!g->isArray || g->dims != 1)
          {
              fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $1);
              exit(1);
          }

          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be an integer\n");
              exit(1);
          }

          struct tnode *arrnode =
              createTree(0, g->type, $1, NULL, NODE_ARRAY, g,
                         $3, NULL, NULL);

          $$ = createTree(0, arrnode->type, NULL, NULL,
                          NODE_ASSGN, NULL, arrnode, $6, NULL);

          free($1);
      }
    | READ '(' ID ')' ';'
      {
          struct Gsymbol *g = Lookup($3);

          if (g == NULL)
          {
              fprintf(stderr,
                      "Error: variable '%s' is not declared\n",
                      $3);
              exit(1);
          }

          if (g->isArray)
          {
              fprintf(stderr,
                      "Error: array '%s' must be indexed\n",
                      $3);
              exit(1);
          }

          struct tnode *idnode =
              createTree(
                  0,
                  g->type,
                  $3,
                  NULL,
                  NODE_ID,
                  g,
                  NULL,
                  NULL,
                  NULL
              );

          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_READ,
              NULL,
              idnode, NULL, NULL
          );

          free($3);
      }
    | READ '(' ID '[' E ']' '[' E ']' ')' ';'
      {
          struct Gsymbol *g = Lookup($3);

          if (g == NULL)
          {
              fprintf(stderr, "Error: variable '%s' is not declared\n", $3);
              exit(1);
          }

          if (!g->isArray || g->dims != 2)
          {
              fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $3);
              exit(1);
          }

          if ($5->type != TYPE_INT || $8->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be an integer\n");
              exit(1);
          }

          struct tnode *arrnode =
              createTree(0, g->type, $3, NULL, NODE_ARRAY, g,
                         $5, $8, NULL);

          $$ = createTree(0, -1, NULL, NULL,
                          NODE_READ, NULL, arrnode, NULL, NULL);

          free($3);
      }
    | READ '(' ID '[' E ']' ')' ';'
      {
          struct Gsymbol *g = Lookup($3);

          if (g == NULL)
          {
              fprintf(stderr, "Error: variable '%s' is not declared\n", $3);
              exit(1);
          }

          if (!g->isArray || g->dims != 1)
          {
              fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $3);
              exit(1);
          }

          if ($5->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be an integer\n");
              exit(1);
          }

          struct tnode *arrnode =
              createTree(0, g->type, $3, NULL, NODE_ARRAY, g,
                         $5, NULL, NULL);

          $$ = createTree(0, -1, NULL, NULL,
                          NODE_READ, NULL, arrnode, NULL, NULL);

          free($3);
      }
    | WRITE '(' E ')' ';'
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_WRITE,
              NULL,
              $3, NULL, NULL
          );
      }
    | IF '(' E ')' THEN Slist ENDIF ';'
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_IF,
              NULL,
              $3, $6, NULL
          );
      }
    | IF '(' E ')' THEN Slist ELSE Slist ENDIF ';'
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_IF,
              NULL,
              $3, $6, $8
          );
      }
    | WHILE '(' E ')' DO Slist ENDWHILE ';'
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_WHILE,
              NULL,
              $3, $6, NULL
          );
      }
    | BREAK ';'
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_BREAK,
              NULL,
              NULL, NULL, NULL
          );
      }
    | CONTINUE ';'
      {
          $$ = createTree(
              0, -1, NULL, NULL,
              NODE_CONTINUE,
              NULL,
              NULL, NULL, NULL
          );
      }
    ;

E:
      E PLUS E
      {
          $$ = createTree(
              0, TYPE_INT, NULL, NULL,
              NODE_PLUS,
              NULL,
              $1, $3, NULL
          );
      }
    | E MINUS E
      {
          $$ = createTree(
              0, TYPE_INT, NULL, NULL,
              NODE_MINUS,
              NULL,
              $1, $3, NULL
          );
      }
    | E MUL E
      {
          $$ = createTree(
              0, TYPE_INT, NULL, NULL,
              NODE_MUL,
              NULL,
              $1, $3, NULL
          );
      }
    | E DIV E
      {
          $$ = createTree(
              0, TYPE_INT, NULL, NULL,
              NODE_DIV,
              NULL,
              $1, $3, NULL
          );
      }
    | E MOD E
      {
          $$ = createTree(
              0, TYPE_INT, NULL, NULL,
              NODE_MOD,
              NULL,
              $1, $3, NULL
          );
      }
    | E LT E
      {
          $$ = createTree(
              0, TYPE_BOOL, NULL, NULL,
              NODE_LT,
              NULL,
              $1, $3, NULL
          );
      }
    | E GT E
      {
          $$ = createTree(
              0, TYPE_BOOL, NULL, NULL,
              NODE_GT,
              NULL,
              $1, $3, NULL
          );
      }
    | E LE E
      {
          $$ = createTree(
              0, TYPE_BOOL, NULL, NULL,
              NODE_LE,
              NULL,
              $1, $3, NULL
          );
      }
    | E GE E
      {
          $$ = createTree(
              0, TYPE_BOOL, NULL, NULL,
              NODE_GE,
              NULL,
              $1, $3, NULL
          );
      }
    | E EQ E
      {
          $$ = createTree(
              0, TYPE_BOOL, NULL, NULL,
              NODE_EQ,
              NULL,
              $1, $3, NULL
          );
      }
    | E NE E
      {
          $$ = createTree(
              0, TYPE_BOOL, NULL, NULL,
              NODE_NE,
              NULL,
              $1, $3, NULL
          );
      }
    | '(' E ')'
      {
          $$ = $2;
      }
    | NUM
      {
          $$ = $1;
      }
    | STRCONST
      {
          $$ = $1;
      }
    | ID
      {
          struct Gsymbol *g = Lookup($1);

          if (g == NULL)
          {
              fprintf(stderr,
                      "Error: variable '%s' is not declared\n",
                      $1);
              exit(1);
          }

          if (g->isArray)
          {
              fprintf(stderr,
                      "Error: array '%s' must be indexed\n",
                      $1);
              exit(1);
          }

          $$ = createTree(
              0,
              g->type,
              $1,
              NULL,
              NODE_ID,
              g,
              NULL,
              NULL,
              NULL
          );

          free($1);
      }
    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);

          if (g == NULL)
          {
              fprintf(stderr, "Error: variable '%s' is not declared\n", $1);
              exit(1);
          }

          if (!g->isArray || g->dims != 2)
          {
              fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $1);
              exit(1);
          }

          if ($3->type != TYPE_INT || $6->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be an integer\n");
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY, g,
                          $3, $6, NULL);

          free($1);
      }
    | ID '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);

          if (g == NULL)
          {
              fprintf(stderr, "Error: variable '%s' is not declared\n", $1);
              exit(1);
          }

          if (!g->isArray || g->dims != 1)
          {
              fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $1);
              exit(1);
          }

          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be an integer\n");
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY, g,
                          $3, NULL, NULL);

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
    if (argc != 2)
    {
        printf("Usage: %s <input.expl>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "r");

    if (!fp)
    {
        perror(argv[1]);
        return 1;
    }

    target = fopen("target.xsm", "w");

    if (!target)
    {
        perror("target.xsm");
        fclose(fp);
        return 1;
    }

    /*
     * XEXE header:
     * magic number, entry point, and five unused fields.
     */
    fprintf(
        target,
        "0\n2056\n0\n0\n0\n0\n0\n0\n"
    );

    yyin = fp;

    if (yyparse() != 0)
    {
        fclose(fp);
        fclose(target);
        return 1;
    }

    emit_exit(target);

    fclose(fp);
    fclose(target);

    return 0;
}