%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symboltable.h"

int yylex(void);
void yyerror(char const *s);
extern FILE* yyin;

int DeclType = -1;
int current_decl_type = -1;
struct Paramstruct* current_param_list = NULL;
struct Typetable *current_type = NULL;
int current_return_type = inttype;

void addParam(char* name, int type);
static struct tnode *lookupVariableNode(struct tnode *id);
static struct tnode *lookupArrayNode(struct tnode *id, struct tnode *index);
static struct tnode *makeField(struct tnode *base, struct tnode *member);
static void checkAssignment(struct tnode *lhs, struct tnode *rhs);
static void checkCallArguments(struct Gsymbol *function, struct tnode *args);
static void checkReturn(struct tnode *expr);
static void checkArithmeticOperands(struct tnode *left, struct tnode *right);
static void checkRelationalOperands(struct tnode *left, struct tnode *right);
void PrintGST(void);
void PrintLST(const char* scopeName);
%}

%union {
    struct tnode *no;
}

%token DECL ENDDECL INT STR MAIN RETURN
%token START END READ WRITE IF THEN ELSE ENDIF WHILE DO ENDWHILE BREAK CONTINUE
%token TYPE ENDTYPE ALLOC FREE INITIALIZE NULLTOK
%token <no> NUM ID STRING
%token LT GT LE GE EQ NE

%type <no> E Slist Stmt RetStmt Variable Field ArgList

%left LT GT LE GE EQ NE
%left '+' '-'
%left '*' '/' '%'
%right '&' USTAR

%%

Program : TypeDefBlock GDeclBlock FDefBlock MainBlock {
            PrintGST();
            exit(0);
        }
        | TypeDefBlock GDeclBlock MainBlock {
            PrintGST();
            exit(0);
        }
        ;

TypeDefBlock : TYPE TypeDefList ENDTYPE
             |
             ;

TypeDefList : TypeDefList TypeDef
            | TypeDef
            ;

TypeDef : ID '{' {
              current_type = TBegin($1->varname);
          } FieldDeclList '}' {
              TFinish(current_type);
              current_type = NULL;
          }
        ;

FieldDeclList : FieldDeclList FieldDecl
              | FieldDecl
              ;

FieldDecl : Type ID ';' {
                if (current_type == NULL) {
                    fprintf(stderr, "Semantic Error: Field outside type definition.\n");
                    exit(1);
                }
                FInstall(current_type, $2->varname, DeclType);
            }
          ;

GDeclBlock : DECL GDeclList ENDDECL
           | DECL ENDDECL
           |
           ;

GDeclList : GDeclList GDecl
          | GDecl
          ;

GDecl : Type { current_decl_type = DeclType; } GIdList ';' ;

Type : INT { DeclType = inttype; }
     | STR { DeclType = strtype; }
     | ID {
           struct Typetable *t = TLookup($1->varname);
           if (t == NULL || t->type < 6 || (!t->defined && (current_type == NULL || t != current_type))) {
               fprintf(stderr, "Semantic Error: Unknown type '%s'.\n", $1->varname);
               exit(1);
           }
           DeclType = t->type;
       }
     ;

GIdList : GIdList ',' GId
        | GId
        ;

GId : ID {
          int size = 1;
          GInstall($1->varname, current_decl_type, size, -2, NULL);
      }
    | '*' ID {
          if (IsUserType(current_decl_type)) { fprintf(stderr, "Type Error: Explicit pointers to user-defined types are not supported.\n"); exit(1); }
          GInstall($2->varname, current_decl_type + 2, 1, -2, NULL);
      }
    | ID '[' NUM ']' {
          if ($3->val <= 0) { fprintf(stderr, "Semantic Error: Array size must be positive.\n"); exit(1); }
          GInstall($1->varname, current_decl_type, $3->val, -1, NULL);
      }
    | ID '[' NUM ']' '[' NUM ']' {
          if (IsUserType(current_decl_type)) {
              fprintf(stderr, "Semantic Error: ExpL supports only one-dimensional arrays of user-defined types.\n");
              exit(1);
          }
          if ($3->val <= 0 || $6->val <= 0) { fprintf(stderr, "Semantic Error: Array dimensions must be positive.\n"); exit(1); }
          GInstall($1->varname, current_decl_type, $3->val * $6->val, $6->val, NULL);
      }
    | ID '(' ParamList ')' {
          GInstall($1->varname, current_decl_type, -1, -1, current_param_list);
          current_param_list = NULL;
      }
    ;

ParamList : ParamList ',' Param
          | Param
          | { current_param_list = NULL; }
          ;

Param : Type ID { addParam($2->varname, DeclType); }
      | Type '*' ID {
            if (IsUserType(DeclType)) { fprintf(stderr, "Type Error: Explicit pointers to user-defined types are not supported.\n"); exit(1); }
            addParam($3->varname, DeclType + 2);
        }
      ;

LDeclBlock : DECL LDecList ENDDECL
           | DECL ENDDECL
           |
           ;

LDecList : LDecList LDecl
         | LDecl
         ;

LDecl : Type { current_decl_type = DeclType; } IdList ';' ;

IdList : IdList ',' ID { LInstall($3->varname, current_decl_type, 0); }
       | IdList ',' '*' ID {
             if (IsUserType(current_decl_type)) { fprintf(stderr, "Type Error: Explicit pointers to user-defined types are not supported.\n"); exit(1); }
             LInstall($4->varname, current_decl_type + 2, 0);
         }
       | ID { LInstall($1->varname, current_decl_type, 0); }
       | '*' ID {
             if (IsUserType(current_decl_type)) { fprintf(stderr, "Type Error: Explicit pointers to user-defined types are not supported.\n"); exit(1); }
             LInstall($2->varname, current_decl_type + 2, 0);
         }
       ;

FDefBlock : FDefBlock FDef
          | FDef
          ;

FDef : Type ID {
          current_return_type = DeclType;
      } '(' ParamList ')' '{' {
          VerifyFunctionSignature($2->varname, current_return_type, current_param_list);
          struct Paramstruct* temp = current_param_list;
          while (temp != NULL) {
              LInstall(temp->name, temp->type, 1);
              temp = temp->next;
          }
      } LDeclBlock START Slist RetStmt END '}' {
          struct tnode* body = createTree(0, -1, NULL, NODE_CONN, NULL, $11, NULL, $12);
          FILE* out = fopen("output.xsm", "a");
          if (!out) { perror("output.xsm"); exit(1); }
          generateFuncCode($2->varname, body, out);
          fclose(out);
          PrintLST($2->varname);
          ClearLST();
          current_param_list = NULL;
      }
    ;

MainBlock : INT MAIN '(' ')' '{' {
          current_return_type = inttype;
          GInstall("main", inttype, -1, -1, NULL);
      } LDeclBlock START Slist RetStmt END '}' {
          struct tnode* body = createTree(0, -1, NULL, NODE_CONN, NULL, $9, NULL, $10);
          FILE* out = fopen("output.xsm", "a");
          if (!out) { perror("output.xsm"); exit(1); }
          generateFuncCode("main", body, out);
          fclose(out);
          PrintLST("main");
          ClearLST();

          FILE* header = fopen("header.xsm", "w");
          if (!header) { perror("header.xsm"); exit(1); }
          fprintf(header, "0\n2056\n0\n0\n0\n0\n0\n0\n");
          extern int memory_binding;
          fprintf(header, "MOV SP, %d\n", memory_binding);
          fprintf(header, "MOV R0, 0\n");
          for (struct Gsymbol *global = Ghead; global != NULL; global = global->next) {
              if (global->size >= 0 && IsUserType(global->type)) {
                  for (int i = 0; i < global->size; i++)
                      fprintf(header, "MOV [%d], R0\n", global->binding + i);
              }
          }
          fprintf(header, "PUSH R0\n");
          struct Gsymbol* main_entry = GLookUp("main");
          fprintf(header, "CALL F%d\n", main_entry->flabel);
          fprintf(header, "MOV R0, \"Exit\"\nPUSH R0\n");
          fprintf(header, "PUSH R0\nPUSH R0\nPUSH R0\nPUSH R0\nCALL 0\n");
          fclose(header);
          system("cat header.xsm output.xsm > temp.xsm && mv temp.xsm output.xsm && rm header.xsm");
      }
    ;

RetStmt : RETURN E ';' {
              checkReturn($2);
              $$ = createTree(0, -1, NULL, NODE_RET, NULL, $2, NULL, NULL);
          }
        ;

Slist : Slist Stmt { $$ = createTree(0, -1, NULL, NODE_CONN, NULL, $1, NULL, $2); }
      | Stmt { $$ = $1; }
      ;

Stmt : Variable '=' E ';' {
           checkAssignment($1, $3);
           $$ = createTree(0, -1, NULL, NODE_ASSIGN, "=", $1, NULL, $3);
       }
     | Variable '=' ALLOC '(' ')' ';' {
           if (!IsUserType($1->type)) {
               fprintf(stderr, "Type Error: alloc() can only be assigned to a user-defined type variable.\n");
               exit(1);
           }
           struct tnode *allocation = createTree(0, $1->type, NULL, NODE_ALLOC, NULL, NULL, NULL, NULL);
           $$ = createTree(0, -1, NULL, NODE_ASSIGN, "=", $1, NULL, allocation);
       }
     | READ '(' Variable ')' ';' {
           if ($3->type != inttype && $3->type != strtype) {
               fprintf(stderr, "Type Error: read() requires an int or str variable/field.\n");
               exit(1);
           }
           $$ = createTree(0, -1, NULL, NODE_READ, NULL, $3, NULL, NULL);
       }
     | WRITE '(' E ')' ';' {
           if ($3->type != inttype && $3->type != strtype) {
               fprintf(stderr, "Type Error: write() requires an int or str expression.\n");
               exit(1);
           }
           $$ = createTree(0, -1, NULL, NODE_WRITE, NULL, $3, NULL, NULL);
       }
     | IF '(' E ')' THEN Slist ELSE Slist ENDIF ';' { $$ = createTree(0, -1, NULL, NODE_IF, NULL, $3, $6, $8); }
     | IF '(' E ')' THEN Slist ENDIF ';' { $$ = createTree(0, -1, NULL, NODE_IF, NULL, $3, $6, NULL); }
     | WHILE '(' E ')' DO Slist ENDWHILE ';' { $$ = createTree(0, -1, NULL, NODE_WHILE, NULL, $3, $6, NULL); }
     | BREAK ';' { $$ = createTree(0, -1, NULL, NODE_BREAK, NULL, NULL, NULL, NULL); }
     | CONTINUE ';' { $$ = createTree(0, -1, NULL, NODE_CONTINUE, NULL, NULL, NULL, NULL); }
     | FREE '(' Variable ')' ';' {
           if (!IsUserType($3->type)) { fprintf(stderr, "Type Error: free() requires a user-defined type variable/field.\\n"); exit(1); }
           $$ = createTree(0, -1, NULL, NODE_FREE_STMT, NULL, $3, NULL, NULL);
       }
     | INITIALIZE '(' ')' ';' { $$ = createTree(0, -1, NULL, NODE_INIT, NULL, NULL, NULL, NULL); }
     ;

E : E '+' E { checkArithmeticOperands($1, $3); $$ = createTree(0, inttype, NULL, NODE_OP, "+", $1, NULL, $3); }
  | E '-' E { checkArithmeticOperands($1, $3); $$ = createTree(0, inttype, NULL, NODE_OP, "-", $1, NULL, $3); }
  | E '*' E { checkArithmeticOperands($1, $3); $$ = createTree(0, inttype, NULL, NODE_OP, "*", $1, NULL, $3); }
  | E '/' E { checkArithmeticOperands($1, $3); $$ = createTree(0, inttype, NULL, NODE_OP, "/", $1, NULL, $3); }
  | E '%' E { checkArithmeticOperands($1, $3); $$ = createTree(0, inttype, NULL, NODE_OP, "%", $1, NULL, $3); }

  | E LT E { checkRelationalOperands($1, $3); $$ = createTree(0, booltype, NULL, NODE_LT, "<", $1, NULL, $3); }
  | E GT E { checkRelationalOperands($1, $3); $$ = createTree(0, booltype, NULL, NODE_GT, ">", $1, NULL, $3); }
  | E LE E { checkRelationalOperands($1, $3); $$ = createTree(0, booltype, NULL, NODE_LE, "<=", $1, NULL, $3); }
  | E GE E { checkRelationalOperands($1, $3); $$ = createTree(0, booltype, NULL, NODE_GE, ">=", $1, NULL, $3); }
  | E EQ E {
        if (!($1->type == $3->type || (IsUserType($1->type) && $3->type == nulltype) ||
              (IsUserType($3->type) && $1->type == nulltype))) {
            fprintf(stderr, "Type Error: Equality operands must have matching types.\n"); exit(1);
        }
        $$ = createTree(0, booltype, NULL, NODE_EQ, "==", $1, NULL, $3);
    }
  | E NE E {
        if (!($1->type == $3->type || (IsUserType($1->type) && $3->type == nulltype) ||
              (IsUserType($3->type) && $1->type == nulltype))) {
            fprintf(stderr, "Type Error: Inequality operands must have matching types.\n"); exit(1);
        }
        $$ = createTree(0, booltype, NULL, NODE_NE, "!=", $1, NULL, $3);
    }

  | '&' Variable {
        int ptr_type = -1;
        if ($2->type == inttype) ptr_type = intptrtype;
        else if ($2->type == strtype) ptr_type = strptrtype;
        if (ptr_type == -1) { fprintf(stderr, "Type Error: address-of supports int/str variables only.\n"); exit(1); }
        $$ = createTree(0, ptr_type, NULL, NODE_AMPER, NULL, $2, NULL, NULL);
    }
  | '(' E ')' { $$ = $2; }
  | NUM { $$ = $1; }
  | STRING { $$ = $1; }
  | INITIALIZE '(' ')' { $$ = createTree(0, inttype, NULL, NODE_INIT_EXPR, NULL, NULL, NULL, NULL); }
  | NULLTOK { $$ = createTree(0, nulltype, NULL, NODE_NULL, NULL, NULL, NULL, NULL); }
  | Variable { $$ = $1; }
  | FREE '(' Variable ')' {
        if (!IsUserType($3->type)) { fprintf(stderr, "Type Error: free() requires a user-defined type variable/field.\n"); exit(1); }
        $$ = createTree(0, inttype, NULL, NODE_FREE, NULL, $3, NULL, NULL);
    }
  | ID '(' ArgList ')' {
        struct Gsymbol* g = GLookUp($1->varname);
        if (g == NULL || g->size != -1) { fprintf(stderr, "Error: Undefined function '%s'\n", $1->varname); exit(1); }
        checkCallArguments(g, $3);
        $$ = createTree(0, g->type, $1->varname, NODE_FUNCALL, NULL, NULL, NULL, NULL);
        $$->Gentry = g;
        $$->arglist = $3;
    }
  ;

ArgList : ArgList ',' E {
              struct tnode* temp = $1;
              while (temp->argnext != NULL) temp = temp->argnext;
              temp->argnext = $3;
              $$ = $1;
          }
        | E { $$ = $1; }
        | { $$ = NULL; }
        ;

Variable : ID { $$ = lookupVariableNode($1); }
         | ID '[' E ']' {
              struct Gsymbol *entry = GLookUp($1->varname);
              if (entry == NULL || entry->size <= 0 || entry->col_size != -1) {
                  fprintf(stderr, "Error: '%s' is not a one-dimensional array.\n", $1->varname); exit(1);
              }
              $$ = createTree(0, entry->type, $1->varname, NODE_ARRAY, NULL, $3, NULL, NULL);
              $$->Gentry = entry;
          }
         | ID '[' E ']' '[' E ']' {
              struct Gsymbol *entry = GLookUp($1->varname);
              if (entry == NULL || entry->size <= 0 || entry->col_size <= 0) {
                  fprintf(stderr, "Error: '%s' is not a two-dimensional array.\n", $1->varname); exit(1);
              }
              $$ = createTree(0, entry->type, $1->varname, NODE_ARRAY2D, NULL, $3, $6, NULL);
              $$->Gentry = entry;
          }
         | '*' E %prec USTAR {
              int base_type = -1;
              if ($2->type == intptrtype) base_type = inttype;
              else if ($2->type == strptrtype) base_type = strtype;
              else { fprintf(stderr, "Type Error: Cannot dereference a non-pointer.\n"); exit(1); }
              $$ = createTree(0, base_type, NULL, NODE_DEREF, NULL, $2, NULL, NULL);
          }
         | Field { $$ = $1; }
         ;

Field : ID '.' ID {
            struct tnode *base = lookupVariableNode($1);
            $$ = makeField(base, $3);
        }
      | ID '[' E ']' '.' ID {
            struct tnode *base = lookupArrayNode($1, $3);
            $$ = makeField(base, $6);
        }
      | Field '.' ID {
            $$ = makeField($1, $3);
        }
      ;

%%

static struct tnode *lookupVariableNode(struct tnode *id) {
    struct Lsymbol* local = LLookUp(id->varname);
    if (local) {
        struct tnode *n = createTree(0, local->type, id->varname, NODE_ID, NULL, NULL, NULL, NULL);
        n->Lentry = local;
        return n;
    }
    struct Gsymbol* global = GLookUp(id->varname);
    if (!global || global->size < 0 || global->col_size != -2) {
        fprintf(stderr, "Error: Undeclared variable or array requires an index: '%s'.\n", id->varname);
        exit(1);
    }
    struct tnode *n = createTree(0, global->type, id->varname, NODE_ID, NULL, NULL, NULL, NULL);
    n->Gentry = global;
    return n;
}

static struct tnode *lookupArrayNode(struct tnode *id, struct tnode *index) {
    struct Gsymbol *entry = GLookUp(id->varname);
    if (!entry || entry->size <= 0 || entry->col_size != -1) {
        fprintf(stderr, "Error: '%s' is not a one-dimensional array.\n", id->varname);
        exit(1);
    }
    if (!IsUserType(entry->type)) {
        fprintf(stderr, "Type Error: Member access through an array requires an array of user-defined type.\n");
        exit(1);
    }
    struct tnode *n = createTree(0, entry->type, id->varname, NODE_ARRAY, NULL, index, NULL, NULL);
    n->Gentry = entry;
    return n;
}

static struct tnode *makeField(struct tnode *base, struct tnode *member) {
    if (!base || !IsUserType(base->type)) {
        fprintf(stderr, "Type Error: Member access requires a user-defined type before '.%s'.\n", member->varname);
        exit(1);
    }
    struct Typetable *owner = TLookupCode(base->type);
    struct Fieldlist *field = FLookup(owner, member->varname);
    if (!field) {
        fprintf(stderr, "Semantic Error: Type '%s' has no member '%s'.\n", owner->name, member->varname);
        exit(1);
    }
    struct tnode *n = createTree(field->fieldIndex, field->type, member->varname, NODE_FIELD, NULL, base, NULL, NULL);
    return n;
}

static void checkArithmeticOperands(struct tnode *left, struct tnode *right) {
    if (!left || !right || left->type != inttype || right->type != inttype) {
        fprintf(stderr, "Type Error: Arithmetic operators require integer operands.\n");
        exit(1);
    }
}

static void checkRelationalOperands(struct tnode *left, struct tnode *right) {
    if (!left || !right || left->type != right->type ||
        (left->type != inttype && left->type != strtype)) {
        fprintf(stderr, "Type Error: Relational operators require matching int or str operands.\n");
        exit(1);
    }
}

static void checkAssignment(struct tnode *lhs, struct tnode *rhs) {
    if (!lhs || !rhs) return;
    if (!IsCompatibleType(lhs->type, rhs->type)) {
        fprintf(stderr, "Type Error: Cannot assign type %d to type %d.\n", rhs->type, lhs->type);
        exit(1);
    }
}

static void checkCallArguments(struct Gsymbol *function, struct tnode *args) {
    struct Paramstruct *p = function->paramlist;
    struct tnode *a = args;
    while (p && a) {
        if (!IsCompatibleType(p->type, a->type)) {
            fprintf(stderr, "Type Error: Argument type mismatch in call to '%s'.\n", function->name);
            exit(1);
        }
        p = p->next;
        a = a->argnext;
    }
    if (p || a) {
        fprintf(stderr, "Type Error: Argument count mismatch in call to '%s'.\n", function->name);
        exit(1);
    }
}

static void checkReturn(struct tnode *expr) {
    if (!expr || !IsCompatibleType(current_return_type, expr->type)) {
        fprintf(stderr, "Type Error: Return expression does not match the function return type.\n");
        exit(1);
    }
}

void yyerror(char const *s) {
    fprintf(stderr, "yyerror: %s\n", s);
}

void addParam(char* name, int type) {
    struct Paramstruct* param = (struct Paramstruct*)malloc(sizeof(struct Paramstruct));
    if (!param) { perror("malloc"); exit(1); }
    param->name = strdup(name);
    param->type = type;
    param->next = NULL;
    if (current_param_list == NULL) current_param_list = param;
    else {
        struct Paramstruct* temp = current_param_list;
        while (temp->next != NULL) temp = temp->next;
        temp->next = param;
    }
}

int main(int argc, char* argv[]) {
    TypeTableCreate();
    if (argc > 1) {
        FILE* fp = fopen(argv[1], "r");
        if (fp) yyin = fp;
        else { perror(argv[1]); return 1; }
    }
    FILE *out = fopen("output.xsm", "w");
    if (!out) { perror("output.xsm"); return 1; }
    fclose(out);
    yyparse();
    return 0;
}