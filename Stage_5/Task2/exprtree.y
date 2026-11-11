%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exprtree.h"
#include "exprtree.c"

int yylex(void);
void yyerror(const char *);
extern FILE *yyin;

FILE *fp;

int currentDeclType;
int currentReturnType;
int currentIsMain = 0;
struct tnode *MainAST = NULL;
struct Lsymbol *MainLhead = NULL;

/* ----------------------- Semantic helpers ----------------------- */

static struct Gsymbol *LookupGlobalVariable(char *name)
{
    struct Gsymbol *g = Lookup(name);

    if (g && g->isFunction)
        return NULL;

    return g;
}

static struct Gsymbol *LookupCallable(char *name)
{
    /* Local declaration suppresses a global declaration with the same name. */
    if (LookupLocal(name) != NULL)
    {
        fprintf(stderr, "Semantic error: '%s' is a local variable and cannot be called as a function\n", name);
        exit(1);
    }

    struct Gsymbol *g = Lookup(name);
    if (!g || !g->isFunction)
    {
        fprintf(stderr, "Semantic error: function '%s' is not declared\n", name);
        exit(1);
    }

    return g;
}

static struct tnode *makeIdentifierNode(char *name)
{
    struct Lsymbol *l = LookupLocal(name);
    if (l)
        return createTree(0, l->type, name, NULL, NODE_ID,
                          NULL, l, NULL, NULL, NULL, NULL, NULL);

    struct Gsymbol *g = LookupGlobalVariable(name);
    if (!g)
    {
        struct Gsymbol *any = Lookup(name);
        if (any && any->isFunction)
            fprintf(stderr, "Semantic error: function '%s' cannot be used as a variable\n", name);
        else
            fprintf(stderr, "Semantic error: variable '%s' is not declared\n", name);
        exit(1);
    }

    if (g->isArray)
    {
        fprintf(stderr, "Semantic error: array '%s' must be indexed\n", name);
        exit(1);
    }

    return createTree(0, g->type, name, NULL, NODE_ID,
                      g, NULL, NULL, NULL, NULL, NULL, NULL);
}

static struct tnode *makeLValueNode(char *name)
{
    struct Lsymbol *l = LookupLocal(name);
    if (l)
        return createTree(0, l->type, name, NULL, NODE_ID,
                          NULL, l, NULL, NULL, NULL, NULL, NULL);

    struct Gsymbol *g = LookupGlobalVariable(name);
    if (!g)
    {
        struct Gsymbol *any = Lookup(name);
        if (any && any->isFunction)
            fprintf(stderr, "Semantic error: function '%s' cannot be assigned to\n", name);
        else
            fprintf(stderr, "Semantic error: variable '%s' is not declared\n", name);
        exit(1);
    }

    if (g->isArray)
    {
        fprintf(stderr, "Semantic error: array '%s' must be indexed\n", name);
        exit(1);
    }

    return createTree(0, g->type, name, NULL, NODE_ID,
                      g, NULL, NULL, NULL, NULL, NULL, NULL);
}

static struct Gsymbol *LookupArray(char *name, int expectedDims)
{
    /* A local symbol with the same name suppresses the global array. */
    if (LookupLocal(name) != NULL)
    {
        fprintf(stderr, "Semantic error: local variable '%s' hides the global array; it cannot be indexed\n", name);
        exit(1);
    }

    struct Gsymbol *g = LookupGlobalVariable(name);
    if (!g)
    {
        fprintf(stderr, "Semantic error: variable '%s' is not declared\n", name);
        exit(1);
    }

    if (!g->isArray || g->dims != expectedDims)
    {
        fprintf(stderr, "Semantic error: '%s' is not the required %d-D array\n", name, expectedDims);
        exit(1);
    }

    return g;
}

static void beginDefinition(char *name, int returnType, struct Paramstruct *defParams)
{
    struct Gsymbol *g = Lookup(name);

    if (!g)
    {
        fprintf(stderr, "Semantic error: function '%s' has no declaration\n", name);
        exit(1);
    }

    if (!g->isFunction)
    {
        fprintf(stderr, "Semantic error: '%s' is a variable, not a function\n", name);
        exit(1);
    }

    if (g->type != returnType)
    {
        fprintf(stderr,
                "Semantic error: return type of function '%s' does not match its declaration\n",
                name);
        exit(1);
    }

    if (g->defined)
    {
        fprintf(stderr, "Semantic error: function '%s' is defined more than once\n", name);
        exit(1);
    }

    compareParams(g->paramlist, defParams, name);
    BeginFunction(g);

    currentReturnType = returnType;
    currentIsMain = 0;
}

static void beginMainDefinition(void)
{
    BeginMain();
    currentReturnType = TYPE_INT;
    currentIsMain = 1;
}

static void printResults(void)
{
    PrintGsymbolTable();

    printf("\n================ LOCAL TABLES + AST ================\n");

    struct Gsymbol *g = Ghead;
    while (g)
    {
        if (g->isFunction && g->defined)
        {
            printf("\nFunction: %s\n", g->name);
            PrintLocalSymbolTable(g->Lhead);
            printf("AST:\n");
            printAST(g->AST, 1);
        }
        g = g->next;
    }

    if (MainAST)
    {
        printf("\nFunction: main\n");
        printf("LOCAL SYMBOL TABLE\n");
        printf("%-14s %-8s %-8s\n", "NAME", "TYPE", "BINDING");
        printf("--------------------------------\n");

        PrintLocalSymbolTable(MainLhead);

        printf("AST:\n");
        printAST(MainAST, 1);
    }

    printf("=====================================================\n");
}

%}

%union
{
    struct tnode *no;
    char *str;
    int type;
    struct Paramstruct *param;
}

%token <no> NUM STRCONST
%token <str> ID
%token DECL ENDDECL INT STR
%token READ WRITE
%token IF THEN ELSE ENDIF
%token DO WHILE ENDWHILE
%token BREAK CONTINUE
%token BEGINBLOCK END RETURN MAIN
%token LT GT LE GE EQ NE
%token MUL DIV MOD PLUS MINUS ASSGN AMP

%type <no> E Slist SlistOpt Stmt LValue Body ReturnStmt ArgList
%type <type> Type PointerStars
%type <param> ParamList Param

%left EQ NE LT GT LE GE
%left PLUS MINUS
%left MUL DIV MOD
%right AMP USTAR

%%

program:
      Declarations FdefBlock MainBlock
      {
          CheckAllFunctionsDefined();
          printResults();
      }
    | Declarations MainBlock
      {
          CheckAllFunctionsDefined();
          printResults();
      }
    | MainBlock
      {
          CheckAllFunctionsDefined();
          printResults();
      }
    ;

/* ------------------------ Global declarations ------------------------ */

Declarations:
      DECL DeclList ENDDECL
    | DECL ENDDECL
    ;

DeclList:
      DeclList Decl
    | Decl
    ;

Type:
      INT { $$ = TYPE_INT; }
    | STR { $$ = TYPE_STR; }
    ;

Decl:
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
              fprintf(stderr, "Semantic error: array '%s' must have positive size\n", $1);
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
              fprintf(stderr, "Semantic error: array '%s' dimensions must be positive\n", $1);
              exit(1);
          }
          Install($1, currentDeclType, rows * cols, 1, 2, rows, cols, 0);
          free($1);
          free($3);
          free($6);
      }
    | ID '(' ParamList ')'
      {
          InstallFunction($1, currentDeclType, $3);
          free($1);
      }
    ;

PointerStars:
      MUL { $$ = 1; }
    | PointerStars MUL { $$ = $1 + 1; }
    ;

/* ------------------------ Function definitions ------------------------ */

FdefBlock:
      FdefBlock Fdef
    | Fdef
    ;

Fdef:
      Type ID '(' ParamList ')'
      {
          beginDefinition($2, $1, $4);
          freeParamList($4);
          free($2);
      }
      '{' LocalDeclarations Body '}'
      {
          CurrentFunction->AST = $9;
          CurrentFunction->defined = 1;
          printf("\n----- Completed function %s -----\n", CurrentFunction->name);
          PrintLocalSymbolTable(CurrentFunction->Lhead);
          EndFunction();
      }
    ;

ParamList:
      /* empty */ { $$ = NULL; }
    | Param { $$ = $1; }
    | ParamList ',' Param
      {
          struct Paramstruct *head = $1;
          appendParam(&head, $3);
          $$ = head;
      }
    ;

Param:
      Type ID
      {
          $$ = createParam($2, $1);
          free($2);
      }
    ;

/* --------------------------- Main definition --------------------------- */

MainBlock:
      INT MAIN '(' ')'
      {
          beginMainDefinition();
      }
      '{' LocalDeclarations Body '}'
      {
          MainAST = createTree(0, TYPE_INT, "main", NULL, NODE_MAIN,
                               NULL, NULL, NULL, $8, NULL, NULL, NULL);

          printf("\n----- Completed function main -----\n");
          MainLhead = Lhead;
          PrintLocalSymbolTable(MainLhead);

          EndFunction();
          currentIsMain = 0;
      }
    ;

/* ------------------------ Local declarations ------------------------ */

LocalDeclarations:
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
          InstallLocal($3, currentDeclType);
          free($3);
      }
    | ID
      {
          InstallLocal($1, currentDeclType);
          free($1);
      }
    ;

/* ------------------------------ Body ------------------------------ */

Body:
      BEGINBLOCK SlistOpt ReturnStmt END
      {
          $$ = createTree(0, currentReturnType, NULL, NULL, NODE_BODY,
                          NULL, NULL, NULL, $2, $3, NULL, NULL);
      }
    ;

ReturnStmt:
      RETURN E ';'
      {
          $$ = createTree(0, currentReturnType, NULL, NULL, NODE_RET,
                          NULL, NULL, NULL, $2, NULL, NULL, NULL);
      }
    ;

SlistOpt:
      /* empty */ { $$ = NULL; }
    | Slist { $$ = $1; }
    ;

Slist:
      Slist Stmt
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_CONNECTOR,
                          NULL, NULL, NULL, $1, $2, NULL, NULL);
      }
    | Stmt
      {
          $$ = $1;
      }
    ;

/* ------------------------------ Statements ------------------------------ */

LValue:
      ID
      {
          $$ = makeLValueNode($1);
          free($1);
      }
    | ID '[' E ']'
      {
          struct Gsymbol *g = LookupArray($1, 1);
          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Semantic error: array index must be int\n");
              exit(1);
          }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, NULL, NULL, NULL);
          free($1);
      }
    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = LookupArray($1, 2);
          if ($3->type != TYPE_INT || $6->type != TYPE_INT)
          {
              fprintf(stderr, "Semantic error: array indices must be int\n");
              exit(1);
          }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, $6, NULL, NULL);
          free($1);
      }
    | MUL E %prec USTAR
      {
          if (!isPointerType($2->type))
          {
              fprintf(stderr, "Semantic error: dereference requires pointer operand\n");
              exit(1);
          }
          $$ = createTree(0,
                          makePtrType(baseTypeOf($2->type), pointerDepth($2->type)-1),
                          NULL, NULL, NODE_DEREF,
                          NULL, NULL, NULL, $2, NULL, NULL, NULL);
      }
    ;

Stmt:
      LValue ASSGN E ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_ASSGN,
                          NULL, NULL, NULL, $1, $3, NULL, NULL);
      }
    | READ '(' LValue ')' ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_READ,
                          NULL, NULL, NULL, $3, NULL, NULL, NULL);
      }
    | WRITE '(' E ')' ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_WRITE,
                          NULL, NULL, NULL, $3, NULL, NULL, NULL);
      }
    | IF '(' E ')' THEN Slist ENDIF ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_IF,
                          NULL, NULL, NULL, $3, $6, NULL, NULL);
      }
    | IF '(' E ')' THEN Slist ELSE Slist ENDIF ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_IF,
                          NULL, NULL, NULL, $3, $6, $8, NULL);
      }
    | WHILE '(' E ')' DO Slist ENDWHILE ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_WHILE,
                          NULL, NULL, NULL, $3, $6, NULL, NULL);
      }
    | BREAK ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_BREAK,
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      }
    | CONTINUE ';'
      {
          $$ = createTree(0, -1, NULL, NULL, NODE_CONTINUE,
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      }
    ;

/* ------------------------------ Expressions ------------------------------ */

E:
      E PLUS E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_PLUS, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E MINUS E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MINUS, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E MUL E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MUL, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E DIV E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_DIV, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E MOD E
      { $$ = createTree(0, TYPE_INT, NULL, NULL, NODE_MOD, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E LT E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_LT, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E GT E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_GT, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E LE E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_LE, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E GE E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_GE, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E EQ E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_EQ, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | E NE E
      { $$ = createTree(0, TYPE_BOOL, NULL, NULL, NODE_NE, NULL, NULL, NULL, $1, $3, NULL, NULL); }
    | AMP ID %prec AMP
      {
          struct Lsymbol *l = LookupLocal($2);
          if (l)
              $$ = createTree(0, makePtrType(l->type, 1), NULL, NULL,
                              NODE_ADDR, NULL, l, NULL, NULL, NULL, NULL, NULL);
          else
          {
              struct Gsymbol *g = LookupGlobalVariable($2);
              if (!g)
              {
                  fprintf(stderr, "Semantic error: variable '%s' is not declared\n", $2);
                  exit(1);
              }
              if (g->isArray)
              {
                  fprintf(stderr, "Semantic error: array '%s' must be indexed\n", $2);
                  exit(1);
              }
              $$ = createTree(0, makePtrType(g->type, 1), NULL, NULL,
                              NODE_ADDR, g, NULL, NULL, NULL, NULL, NULL, NULL);
          }
          free($2);
      }
    | MUL E %prec USTAR
      {
          if (!isPointerType($2->type))
          {
              fprintf(stderr, "Semantic error: dereference requires pointer operand\n");
              exit(1);
          }
          $$ = createTree(0,
                          makePtrType(baseTypeOf($2->type), pointerDepth($2->type)-1),
                          NULL, NULL, NODE_DEREF,
                          NULL, NULL, NULL, $2, NULL, NULL, NULL);
      }
    | '(' E ')' { $$ = $2; }
    | NUM { $$ = $1; }
    | STRCONST { $$ = $1; }
    | ID
      {
          $$ = makeIdentifierNode($1);
          free($1);
      }
    | ID '[' E ']'
      {
          struct Gsymbol *g = LookupArray($1, 1);
          if ($3->type != TYPE_INT)
          {
              fprintf(stderr, "Semantic error: array index must be int\n");
              exit(1);
          }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, NULL, NULL, NULL);
          free($1);
      }
    | ID '[' E ']' '[' E ']'
      {
          struct Gsymbol *g = LookupArray($1, 2);
          if ($3->type != TYPE_INT || $6->type != TYPE_INT)
          {
              fprintf(stderr, "Semantic error: array indices must be int\n");
              exit(1);
          }
          $$ = createTree(0, g->type, $1, NULL, NODE_ARRAY,
                          g, NULL, NULL, $3, $6, NULL, NULL);
          free($1);
      }
    | ID '(' ')'
      {
          struct Gsymbol *g = LookupCallable($1);
          $$ = createCallTree(g, NULL);
          free($1);
      }
    | ID '(' ArgList ')'
      {
          struct Gsymbol *g = LookupCallable($1);
          $$ = createCallTree(g, $3);
          free($1);
      }
    ;

ArgList:
      E { $$ = $1; }
    | ArgList ',' E
      {
          $$ = appendArg($1, $3);
      }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "Syntax error: %s\n", s);
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

    yyin = fp;

    if (yyparse() != 0)
    {
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}