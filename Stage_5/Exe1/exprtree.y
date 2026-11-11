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

/* Main's AST/LST are stored separately because main has no GST declaration. */
%}

%union
{
    struct tnode *no;
    char *str;
    int type;
    struct Gsymbol *g;
    struct Lsymbol *l;
    struct Paramstruct *param;
    struct Arglist *args;
}

%token <no> NUM STRCONST
%token <str> ID
%token DECL ENDDECL INT STR
%token READ WRITE
%token IF THEN ELSE ENDIF
%token DO WHILE ENDWHILE
%token BREAK CONTINUE
%token RETURN MAIN VOID
%token LT GT LE GE EQ NE
%token AND OR NOT
%token MUL DIV MOD PLUS MINUS ASSGN AMP
%token BEGINBLOCK END

%type <no> E Slist SlistOpt Stmt LValue ReturnStmt
%type <type> Type PointerStars
%type <param> Param ParamList ParamListOpt
%type <args> ArgList ArgListOpt
%type <g> FHeader

%left OR
%left AND
%left EQ NE LT GT LE GE
%left PLUS MINUS
%left MUL DIV MOD
%right NOT AMP USTAR

%%

program:
      GDeclBlock FdefBlock MainBlock
    | GDeclBlock MainBlock
    | MainBlock
    ;

GDeclBlock:
      DECL GdeclList ENDDECL
    | DECL ENDDECL
    ;

GdeclList:
      GdeclList Gdecl
    | Gdecl
    ;

Gdecl:
      Type
      {
          currentDeclType = $1;
      }
      GidList ';'
      ;

GidList:
      GidList ',' Gid
    | Gid
    ;

Gid:
      ID
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
          if (n <= 0)
          {
              fprintf(stderr, "Error: array '%s' must have positive size\n", $1);
              exit(1);
          }

          Install($1, currentDeclType, n, 1, 1, n, 1, 0);
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

          Install($1, currentDeclType, rows * cols, 1, 2, rows, cols, 0);
          free($1);
          free($3);
          free($6);
      }
    | ID '(' ParamListOpt ')'
      {
          InstallFunction($1, currentDeclType, $3);
          free($1);
      }
    | PointerStars ID '(' ParamListOpt ')'
      {
          int returnType = makePtrType(currentDeclType, $1);
          InstallFunction($2, returnType, $4);
          free($2);
      }
    ;

Type:
      INT { $$ = TYPE_INT; }
    | STR { $$ = TYPE_STR; }
    ;

PointerStars:
      MUL { $$ = 1; }
    | PointerStars MUL { $$ = $1 + 1; }
    ;

ParamListOpt:
      /* empty */ { $$ = NULL; }
    | ParamList { $$ = $1; }
    | VOID { $$ = NULL; }
    ;

ParamList:
      ParamList ',' Param
      {
          AppendParam(&$1, $3);
          $$ = $1;
      }
    | Param
      {
          $$ = $1;
      }
    ;

Param:
      Type ID
      {
          $$ = CreateParam($2, $1);
          free($2);
      }
    | Type PointerStars ID
      {
          $$ = CreateParam($3, makePtrType($1, $2));
          free($3);
      }
    ;

FdefBlock:
      FdefBlock Fdef
    | Fdef
    ;

Fdef:
      FHeader '{' LDeclBlockOpt BEGINBLOCK SlistOpt ReturnStmt END '}'
      {
          /* $1 is the GST entry selected by FHeader. */
          $1->AST = createTree(0, -1, NULL, NULL, NODE_CONNECTOR,
                               NULL, NULL, NULL, $5, $6, NULL);
          $1->defined = 1;

          EndFunction();
          codeGenFunction($1, target);

          /* Task 3 permits us to discard the compile-time local table and AST
             after the function's code has been emitted. */
          freeAST($1->AST);
          $1->AST = NULL;
          freeLocalSymbolTable($1->Lhead);
          $1->Lhead = NULL;
      }
    ;

FHeader:
      Type ID '(' ParamListOpt ')'
      {
          struct Gsymbol *g = Lookup($2);

          if (!g || !g->isFunction)
          {
              fprintf(stderr, "Error: function '%s' must be declared in global declarations\n", $2);
              exit(1);
          }

          if (!CheckSignature(g, $1, $4))
          {
              fprintf(stderr, "Error: function declaration and definition of '%s' do not match\n", $2);
              exit(1);
          }

          BeginFunction(g);
          $$ = g;
          free($2);

          /* The definition has its own ParamList object; the declaration's
             signature remains stored in the GST. */
          freeParamList($4);
      }
    | Type PointerStars ID '(' ParamListOpt ')'
      {
          int returnType = makePtrType($1, $2);
          struct Gsymbol *g = Lookup($3);

          if (!g || !g->isFunction)
          {
              fprintf(stderr, "Error: function '%s' must be declared in global declarations\n", $3);
              exit(1);
          }

          if (!CheckSignature(g, returnType, $5))
          {
              fprintf(stderr, "Error: function declaration and definition of '%s' do not match\n", $3);
              exit(1);
          }

          BeginFunction(g);
          $$ = g;
          free($3);

          freeParamList($5);
      }
    ;

LDeclBlockOpt:
      /* empty */
    | LDeclBlock
    ;

LDeclBlock:
      DECL LDeclList ENDDECL
    | DECL ENDDECL
    ;

LDeclList:
      LDeclList LDecl
    | LDecl
    ;

LDecl:
      Type
      {
          currentDeclType = $1;
      }
      IdList ';'
      ;

IdList:
      IdList ',' ID
      {
          InstallLocal($3, currentDeclType, nextLocalBinding++);
          free($3);
      }
    | IdList ',' PointerStars ID
      {
          InstallLocal($4, makePtrType(currentDeclType, $3), nextLocalBinding++);
          free($4);
      }
    | ID
      {
          InstallLocal($1, currentDeclType, nextLocalBinding++);
          free($1);
      }
    | PointerStars ID
      {
          InstallLocal($2, makePtrType(currentDeclType, $1), nextLocalBinding++);
          free($2);
      }
    ;

MainParamOpt:
      /* empty */
    | VOID
    ;

MainBlock:
      INT MAIN '(' MainParamOpt ')'
      {
          BeginMain();
      }
      '{' LDeclBlockOpt BEGINBLOCK SlistOpt ReturnStmt END '}'
      {
          MainAST = createTree(0, -1, NULL, NULL, NODE_CONNECTOR,
                               NULL, NULL, NULL, $10, $11, NULL);

          MainLhead = Lhead;
          Lhead = NULL;

          codeGenMain(MainAST, MainLhead, target);

          freeAST(MainAST);
          MainAST = NULL;
          freeLocalSymbolTable(MainLhead);
          MainLhead = NULL;
      }
    ;

Slist:
      Slist Stmt
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_CONNECTOR,
                          NULL, NULL, NULL, $1, $2, NULL);
      }
    | Stmt
      {
          $$ = $1;
      }
    ;

SlistOpt:
      /* empty */
      {
          $$ = NULL;
      }
    | Slist
      {
          $$ = $1;
      }
    ;

ReturnStmt:
      RETURN E ';'
      {
          int expected;

          if (CurrentFunction)
              expected = CurrentFunction->type;
          else
              expected = TYPE_INT; /* main */

          if ($2->type != expected)
          {
              fprintf(stderr, "Error: return type mismatch\n");
              exit(1);
          }

          if (CurrentFunction && isPointerType(expected) &&
              $2->nodetype == NODE_ADDR && $2->Lentry != NULL)
          {
              fprintf(stderr, "Warning: returning address of local variable from function '%s'\n",
                      CurrentFunction->name);
          }

          $$ = createTree(0, -1, NULL, NULL, NODE_RETURN,
                          NULL, NULL, NULL, $2, NULL, NULL);
      }
    ;

LValue:
      ID
      {
          struct Gsymbol *g;
          struct Lsymbol *l;
          resolveVariable($1, &g, &l);

          if (g && g->isArray)
          {
              fprintf(stderr, "Error: array '%s' must be indexed\n", $1);
              exit(1);
          }

          $$ = createTree(0, l ? l->type : g->type,
                          $1, NULL, NODE_ID, g, l, NULL,
                          NULL, NULL, NULL);
          free($1);
      }
    | ID '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);

          if (!g || g->isFunction)
          {
              fprintf(stderr, "Error: array '%s' is not declared\n", $1);
              exit(1);
          }
          if (!g->isArray || g->dims != 1)
          {
              fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $1);
              exit(1);
          }
          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be integer\n");
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, NULL, NULL);
          free($1);
      }
    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);

          if (!g || g->isFunction)
          {
              fprintf(stderr, "Error: array '%s' is not declared\n", $1);
              exit(1);
          }
          if (!g->isArray || g->dims != 2)
          {
              fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $1);
              exit(1);
          }
          if ($3->type != TYPE_INT || $6->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array indices must be integer\n");
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, $6, NULL);
          free($1);
      }
    | MUL E %prec USTAR
      {
          if (!isPointerType($2->type))
          {
              fprintf(stderr, "Error: dereference requires a pointer operand\n");
              exit(1);
          }

          $$ = createTree(0,
                          makePtrType(baseTypeOf($2->type), pointerDepth($2->type) - 1),
                          NULL, NULL, NODE_DEREF,
                          NULL, NULL, NULL, $2, NULL, NULL);
      }
    ;

Stmt:
      LValue ASSGN E ';'
      {
          if ($1->nodetype == NODE_ID && $1->Gentry && $1->Gentry->isArray)
          {
              fprintf(stderr, "Error: array must be indexed\n");
              exit(1);
          }

          if ($1->type != $3->type)
          {
              fprintf(stderr, "Type Mismatch in assignment\n");
              exit(1);
          }

          $$ = createTree(0, $1->type, NULL, NULL, NODE_ASSGN,
                          NULL, NULL, NULL, $1, $3, NULL);
      }
    | ID '(' ArgListOpt ')' ';'
      {
          struct Gsymbol *g = LookupFunction($1);

          if (!g)
          {
              fprintf(stderr, "Error: function '%s' is not declared\n", $1);
              exit(1);
          }

          struct Paramstruct *p = g->paramlist;
          struct Arglist *a = $3;

          while (p && a)
          {
              if (p->type != a->expr->type)
              {
                  fprintf(stderr, "Error: argument type mismatch in call to '%s'\n", $1);
                  exit(1);
              }
              p = p->next;
              a = a->next;
          }

          if (p || a)
          {
              fprintf(stderr, "Error: wrong number of arguments in call to '%s'\n", $1);
              exit(1);
          }

          /* A function call can be used as a statement when the return value
             is intentionally ignored. */
          $$ = createTree(0, g->type, $1, NULL, NODE_FUNCALL,
                          g, NULL, $3, NULL, NULL, NULL);
          free($1);
      }
    | READ '(' LValue ')' ';'
      {
          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Type Mismatch: read expects an integer lvalue\n");
              exit(1);
          }

          $$ = createTree(0, -1, NULL, NULL, NODE_READ,
                          NULL, NULL, NULL, $3, NULL, NULL);
      }
    | WRITE '(' E ')' ';'
      {
          if ($3->type != TYPE_INT && $3->type != TYPE_STR)
          {
              fprintf(stderr, "Type Mismatch in write\n");
              exit(1);
          }

          $$ = createTree(0, -1, NULL, NULL, NODE_WRITE,
                          NULL, NULL, NULL, $3, NULL, NULL);
      }
    | IF '(' E ')' THEN Slist ENDIF ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_IF,
                          NULL, NULL, NULL, $3, $6, NULL);
      }
    | IF '(' E ')' THEN Slist ELSE Slist ENDIF ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_IF,
                          NULL, NULL, NULL, $3, $6, $8);
      }
    | WHILE '(' E ')' DO Slist ENDWHILE ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_WHILE,
                          NULL, NULL, NULL, $3, $6, NULL);
      }
    | BREAK ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_BREAK,
                          NULL, NULL, NULL, NULL, NULL, NULL);
      }
    | CONTINUE ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_CONTINUE,
                          NULL, NULL, NULL, NULL, NULL, NULL);
      }
    ;

E:
      E PLUS E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_PLUS, NULL, NULL, NULL, $1, $3, NULL); }
    | E MINUS E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MINUS, NULL, NULL, NULL, $1, $3, NULL); }
    | E MUL E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MUL, NULL, NULL, NULL, $1, $3, NULL); }
    | E DIV E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_DIV, NULL, NULL, NULL, $1, $3, NULL); }
    | E MOD E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MOD, NULL, NULL, NULL, $1, $3, NULL); }
    | E LT E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_LT, NULL, NULL, NULL, $1, $3, NULL); }
    | E GT E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_GT, NULL, NULL, NULL, $1, $3, NULL); }
    | E LE E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_LE, NULL, NULL, NULL, $1, $3, NULL); }
    | E GE E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_GE, NULL, NULL, NULL, $1, $3, NULL); }
    | E EQ E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_EQ, NULL, NULL, NULL, $1, $3, NULL); }
    | E NE E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_NE, NULL, NULL, NULL, $1, $3, NULL); }
    | E AND E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_AND, NULL, NULL, NULL, $1, $3, NULL); }
    | E OR E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_OR, NULL, NULL, NULL, $1, $3, NULL); }
    | NOT E %prec NOT
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_NOT, NULL, NULL, NULL, $2, NULL, NULL); }

    | ID '(' ArgListOpt ')'
      {
          struct Gsymbol *g = LookupFunction($1);

          if (!g)
          {
              fprintf(stderr, "Error: function '%s' is not declared\n", $1);
              exit(1);
          }

          struct Paramstruct *p = g->paramlist;
          struct Arglist *a = $3;

          while (p && a)
          {
              if (p->type != a->expr->type)
              {
                  fprintf(stderr, "Error: argument type mismatch in call to '%s'\n", $1);
                  exit(1);
              }
              p = p->next;
              a = a->next;
          }

          if (p || a)
          {
              fprintf(stderr, "Error: wrong number of arguments in call to '%s'\n", $1);
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_FUNCALL,
                          g, NULL, $3, NULL, NULL, NULL);
          free($1);
      }

    | AMP ID %prec AMP
      {
          struct Gsymbol *g;
          struct Lsymbol *l;
          resolveVariable($2, &g, &l);

          if (g && g->isArray)
          {
              fprintf(stderr, "Error: use an indexed array element with &\n");
              exit(1);
          }

          if (l)
          {
              $$ = createTree(0,
                              makePtrType(baseTypeOf(l->type), pointerDepth(l->type) + 1),
                              NULL, NULL, NODE_ADDR,
                              NULL, l, NULL, NULL, NULL, NULL);
          }
          else
          {
              $$ = createTree(0,
                              makePtrType(g->baseType, g->ptrDepth + 1),
                              NULL, NULL, NODE_ADDR,
                              g, NULL, NULL, NULL, NULL, NULL);
          }
          free($2);
      }

    | AMP ID '[' E ']' %prec AMP
      {
          struct Gsymbol *g = Lookup($2);

          if (!g || !g->isArray || g->dims != 1)
          {
              fprintf(stderr, "Error: invalid 1D array reference in &\n");
              exit(1);
          }
          if ($4->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be integer\n");
              exit(1);
          }

          struct tnode *arr = createTree(0, g->type, $2, NULL, NODE_ARRAY,
                                         g, NULL, NULL, $4, NULL, NULL);
          $$ = createTree(0, makePtrType(g->baseType, 1), NULL, NULL,
                          NODE_ADDR, NULL, NULL, NULL, arr, NULL, NULL);
          free($2);
      }

    | AMP ID '[' E ']' '[' E ']' %prec AMP
      {
          struct Gsymbol *g = Lookup($2);

          if (!g || !g->isArray || g->dims != 2)
          {
              fprintf(stderr, "Error: invalid 2D array reference in &\n");
              exit(1);
          }
          if ($4->type != TYPE_INT || $7->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array indices must be integer\n");
              exit(1);
          }

          struct tnode *arr = createTree(0, g->type, $2, NULL, NODE_ARRAY,
                                         g, NULL, NULL, $4, $7, NULL);
          $$ = createTree(0, makePtrType(g->baseType, 1), NULL, NULL,
                          NODE_ADDR, NULL, NULL, NULL, arr, NULL, NULL);
          free($2);
      }

    | MUL E %prec USTAR
      {
          if (!isPointerType($2->type))
          {
              fprintf(stderr, "Error: dereference requires a pointer operand\n");
              exit(1);
          }

          $$ = createTree(0,
                          makePtrType(baseTypeOf($2->type), pointerDepth($2->type) - 1),
                          NULL, NULL, NODE_DEREF,
                          NULL, NULL, NULL, $2, NULL, NULL);
      }

    | '(' E ')' { $$ = $2; }
    | NUM { $$ = $1; }
    | STRCONST { $$ = $1; }

    | ID
      {
          struct Gsymbol *g;
          struct Lsymbol *l;
          resolveVariable($1, &g, &l);

          if (g && g->isArray)
          {
              fprintf(stderr, "Error: array '%s' must be indexed\n", $1);
              exit(1);
          }

          $$ = createTree(0,
                          l ? l->type : g->type,
                          $1, NULL, NODE_ID,
                          g, l, NULL, NULL, NULL, NULL);
          free($1);
      }

    | ID '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);

          if (!g || g->isFunction || !g->isArray || g->dims != 1)
          {
              fprintf(stderr, "Error: '%s' is not a one-dimensional array\n", $1);
              exit(1);
          }
          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array index must be integer\n");
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, NULL, NULL);
          free($1);
      }

    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = Lookup($1);

          if (!g || g->isFunction || !g->isArray || g->dims != 2)
          {
              fprintf(stderr, "Error: '%s' is not a two-dimensional array\n", $1);
              exit(1);
          }
          if ($3->type != TYPE_INT || $6->type != TYPE_INT)
          {
              fprintf(stderr, "Error: array indices must be integer\n");
              exit(1);
          }

          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, $6, NULL);
          free($1);
      }
    ;

ArgListOpt:
      /* empty */ { $$ = NULL; }
    | ArgList { $$ = $1; }
    ;

ArgList:
      ArgList ',' E
      {
          $$ = AppendArg($1, $3);
      }
    | E
      {
          $$ = AppendArg(NULL, $1);
      }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "error: %s\n", s);
    exit(1);
}

static void checkAllFunctionsDefined(void)
{
    struct Gsymbol *g = Ghead;

    while (g)
    {
        if (g->isFunction && !g->defined)
        {
            fprintf(stderr, "Error: function '%s' declared but not defined\n", g->name);
            exit(1);
        }
        g = g->next;
    }
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

    /* Standard XEXE header. Entry point remains 2056. */
    fprintf(target, "0\n2056\n0\n0\n0\n0\n0\n0\n");

    /* Function definitions are emitted before main; jump over them at entry. */
    fprintf(target, "JMP MAIN\n");

    yyin = fp;

    if (yyparse() != 0)
    {
        fclose(fp);
        fclose(target);
        return 1;
    }

    checkAllFunctionsDefined();

    fclose(fp);
    fclose(target);
    return 0;
}