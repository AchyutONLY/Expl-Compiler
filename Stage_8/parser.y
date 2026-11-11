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
struct Classtable *DeclClassType = NULL;
struct Classtable *current_decl_class = NULL;
struct Paramstruct* current_param_list = NULL;
struct Typetable *current_type = NULL;
struct Classtable *current_class = NULL;
struct Memberfunclist *current_method = NULL;
int current_return_type = inttype;
int in_class_method = 0;
int current_class_member_type = -1;
struct Classtable *current_class_member_class_type = NULL;
char *current_class_member_type_name = NULL;
char *current_class_member_name = NULL;

static void rememberClassMemberType(void) {
    free(current_class_member_type_name);
    current_class_member_type_name = NULL;
    current_class_member_type = DeclType;
    current_class_member_class_type = DeclClassType;

    if (DeclClassType != NULL) {
        current_class_member_type_name = strdup(DeclClassType->name);
    } else if (DeclType == inttype) {
        current_class_member_type_name = strdup("int");
    } else if (DeclType == strtype) {
        current_class_member_type_name = strdup("str");
    } else {
        struct Typetable *t = TLookupCode(DeclType);
        if (t == NULL) {
            fprintf(stderr, "Semantic Error: Cannot resolve class member type.\n");
            exit(1);
        }
        current_class_member_type_name = strdup(t->name);
    }
    if (current_class_member_type_name == NULL) {
        perror("strdup");
        exit(1);
    }
}

static void clearClassMemberContext(void) {
    free(current_class_member_type_name);
    free(current_class_member_name);
    current_class_member_type_name = NULL;
    current_class_member_name = NULL;
    current_class_member_type = -1;
    current_class_member_class_type = NULL;
}

void addParam(char* name, int type);
static struct tnode *lookupVariableNode(struct tnode *id);
static struct tnode *lookupArrayNode(struct tnode *id, struct tnode *index);
static struct tnode *makeField(struct tnode *base, struct tnode *member);
static struct tnode *makeClassField(struct tnode *base, struct tnode *member);
static struct tnode *makeSelfNode(void);
static struct tnode *makeMethodCall(struct tnode *receiver, struct tnode *method_id, struct tnode *args);
static int expressionsCompatible(const struct tnode *lhs, const struct tnode *rhs);
static void checkAssignment(struct tnode *lhs, struct tnode *rhs);
static void checkCallArguments(struct Gsymbol *function, struct tnode *args);
static void checkMethodArguments(struct Memberfunclist *method, struct tnode *args);
static struct Memberfunclist *resolveMethod(struct Classtable *ctype, const char *name, struct tnode *args);
static void checkReturn(struct tnode *expr);
static void checkArithmeticOperands(struct tnode *left, struct tnode *right);
static void checkRelationalOperands(struct tnode *left, struct tnode *right);
void PrintGST(void);
void PrintLST(const char* scopeName);
void PrintClassTable(void);
%}

%union {
    struct tnode *no;
}

%token DECL ENDDECL INT STR MAIN RETURN
%token START END READ WRITE IF THEN ELSE ENDIF WHILE DO ENDWHILE BREAK CONTINUE
%token TYPE ENDTYPE ALLOC FREE INITIALIZE NULLTOK
%token CLASS ENDCLASS SELF NEW EXTENDS
%token <no> NUM ID STRING
%token LT GT LE GE EQ NE

%type <no> E Slist Stmt RetStmt Variable Field FieldFunction ArgList

%left LT GT LE GE EQ NE
%left '+' '-'
%left '*' '/' '%'
%right '&' USTAR

%%

Program : TypeDefBlock ClassDefBlock GDeclBlock FDefBlock MainBlock {
            PrintGST();
            PrintClassTable();
            exit(0);
        }
        | TypeDefBlock ClassDefBlock GDeclBlock MainBlock {
            PrintGST();
            PrintClassTable();
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
                if (DeclClassType != NULL) {
                    fprintf(stderr, "Semantic Error: Record fields cannot have class type.\n");
                    exit(1);
                }
                if (current_type == NULL) {
                    fprintf(stderr, "Semantic Error: Field outside type definition.\n");
                    exit(1);
                }
                FInstall(current_type, $2->varname, DeclType);
            }
          ;

ClassDefBlock : CLASS ClassDefList ENDCLASS
              |
              ;

ClassDefList : ClassDefList ClassDef
             | ClassDef
             ;

ClassDef : ID '{' {
              current_class = CInstall($1->varname, NULL);
          } DECL ClassMemberDeclList ENDDECL ClassMethodDefList '}' {
              for (struct Memberfunclist *m = current_class->methods; m; m = m->next) {
                  if (!m->defined) {
                      fprintf(stderr, "Semantic Error: Method '%s' declared but not defined in class '%s'.\n", m->name, current_class->name);
                      exit(1);
                  }
              }
              current_class = NULL;
              current_method = NULL;
              in_class_method = 0;
          }
        | ID EXTENDS ID '{' {
              current_class = CInstall($1->varname, $3->varname);
          } DECL ClassMemberDeclList ENDDECL ClassMethodDefList '}' {
              for (struct Memberfunclist *m = current_class->methods; m; m = m->next) {
                  if (!m->defined) {
                      fprintf(stderr, "Semantic Error: Method '%s' declared but not defined in class '%s'.\n", m->name, current_class->name);
                      exit(1);
                  }
              }
              current_class = NULL;
              current_method = NULL;
              in_class_method = 0;
          }
        ;

/*
 * Fields and method declarations share the same Type ID prefix. Parse that
 * prefix once, then distinguish a field (';') from a method ('('). Keeping
 * them in separate nullable lists causes shift/reduce conflicts.
 */
ClassMemberDeclList : ClassMemberDeclList ClassMemberDecl
                    |
                    ;

ClassMemberDecl : Type ID {
                    current_class_member_name = strdup($2->varname);
                    if (current_class_member_name == NULL) { perror("strdup"); exit(1); }
                    rememberClassMemberType();
                } ClassMemberTail
                ;

ClassMemberTail : ';' {
                      Class_Finstall(current_class, current_class_member_type_name,
                                     current_class_member_name);
                      clearClassMemberContext();
                  }
                | '(' ParamList ')' ';' {
                      current_return_type = current_class_member_type;
                      if (current_class_member_class_type != NULL) {
                          fprintf(stderr, "Type Error: Methods cannot return class objects.\n");
                          exit(1);
                      }
                      Class_Minstall(current_class, current_class_member_name,
                                     current_return_type, current_param_list);
                      current_param_list = NULL;
                      clearClassMemberContext();
                  }
                ;

ClassMethodDefList : ClassMethodDefList ClassMethodDef
                   |
                   ;

ClassMethodDef : Type ID {
                     current_return_type = DeclType;
                     if (DeclClassType != NULL) {
                         fprintf(stderr, "Type Error: Methods cannot return class objects.\n");
                         exit(1);
                     }
                 } '(' ParamList ')' '{' {
                     current_method = Class_MlookupSignature(current_class, $2->varname, current_param_list);
                     VerifyMethodSignature(current_method, current_return_type, current_param_list);
                     if (current_method->defined) {
                         fprintf(stderr, "Semantic Error: Method '%s' is defined more than once.\n", current_method->name);
                         exit(1);
                     }
                     int param_count = 0;
                     for (struct Paramstruct *p = current_param_list; p; p = p->next) {
                         LInstall(p->name, p->type, 1);
                         param_count++;
                     }
                     LInstallSelf(current_class, -4 - param_count);
                     current_method->defined = 1;
                     in_class_method = 1;
                 } LDeclBlock START Slist RetStmt END '}' {
                     struct tnode *body = createTree(0, -1, NULL, NODE_CONN, NULL, $11, NULL, $12);
                     FILE *out = fopen("output.xsm", "a");
                     if (!out) { perror("output.xsm"); exit(1); }
                     generateMethodCode(current_method, body, out);
                     fclose(out);
                     PrintLST(current_method->name);
                     ClearLST();
                     current_param_list = NULL;
                     current_method = NULL;
                     in_class_method = 0;
                 }
               ;

GDeclBlock : DECL GDeclList ENDDECL
           | DECL ENDDECL
           |
           ;

GDeclList : GDeclList GDecl
          | GDecl
          ;

GDecl : Type { current_decl_type = DeclType; current_decl_class = DeclClassType; } GIdList ';' ;

Type : INT { DeclType = inttype; DeclClassType = NULL; }
     | STR { DeclType = strtype; DeclClassType = NULL; }
     | ID {
           struct Typetable *t = TLookup($1->varname);
           if (t != NULL && t->type >= 6 && (t->defined || (current_type != NULL && t == current_type))) {
               DeclType = t->type;
               DeclClassType = NULL;
           } else {
               struct Classtable *c = CLookup($1->varname);
               if (c != NULL) {
                   DeclType = CLASS_TYPE;
                   DeclClassType = c;
               } else {
                   fprintf(stderr, "Semantic Error: Unknown type '%s'.\n", $1->varname);
                   exit(1);
               }
           }
       }
     ;

GIdList : GIdList ',' GId
        | GId
        ;

GId : ID {
          if (current_decl_class != NULL) GInstallClass($1->varname, current_decl_class);
          else GInstall($1->varname, current_decl_type, 1, -2, NULL);
      }
    | '*' ID {
          if (current_decl_class != NULL) { fprintf(stderr, "Type Error: Explicit pointers to class objects are not supported.\n"); exit(1); }
          if (IsUserType(current_decl_type)) { fprintf(stderr, "Type Error: Explicit pointers to user-defined types are not supported.\n"); exit(1); }
          GInstall($2->varname, current_decl_type + 2, 1, -2, NULL);
      }
    | ID '[' NUM ']' {
          if (current_decl_class != NULL) { fprintf(stderr, "Semantic Error: Arrays of class objects are not supported.\n"); exit(1); }
          if ($3->val <= 0) { fprintf(stderr, "Semantic Error: Array size must be positive.\n"); exit(1); }
          GInstall($1->varname, current_decl_type, $3->val, -1, NULL);
      }
    | ID '[' NUM ']' '[' NUM ']' {
          if (current_decl_class != NULL || IsUserType(current_decl_type)) {
              fprintf(stderr, "Semantic Error: ExpL supports only primitive two-dimensional arrays.\n");
              exit(1);
          }
          if ($3->val <= 0 || $6->val <= 0) { fprintf(stderr, "Semantic Error: Array dimensions must be positive.\n"); exit(1); }
          GInstall($1->varname, current_decl_type, $3->val * $6->val, $6->val, NULL);
      }
    | ID '(' ParamList ')' {
          if (current_decl_class != NULL) { fprintf(stderr, "Type Error: Functions cannot return class objects.\n"); exit(1); }
          GInstall($1->varname, current_decl_type, -1, -1, current_param_list);
          current_param_list = NULL;
      }
    ;

ParamList : ParamList ',' Param
          | Param
          | { current_param_list = NULL; }
          ;

Param : Type ID { if (DeclClassType != NULL) { fprintf(stderr, "Type Error: Class objects cannot be function/method parameters.\n"); exit(1); } addParam($2->varname, DeclType); }
      | Type '*' ID {
            if (DeclClassType != NULL) { fprintf(stderr, "Type Error: Pointers to class objects are not supported.\n"); exit(1); }
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

LDecl : Type { if (DeclClassType != NULL) { fprintf(stderr, "Semantic Error: Class variables cannot be local.\n"); exit(1); } current_decl_type = DeclType; } IdList ';' ;

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
          if (DeclClassType != NULL) {
              fprintf(stderr, "Type Error: Functions cannot return class objects.\n");
              exit(1);
          }
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
          WriteClassVFTInit(header);
          for (struct Gsymbol *global = Ghead; global != NULL; global = global->next) {
              if (global->size >= 0 && (IsUserType(global->type) || global->ctype != NULL)) {
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
      | { $$ = NULL; }
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
           if (!IsUserType($3->type) && $3->Ctype == NULL) { fprintf(stderr, "Type Error: free() requires a user-defined type or class object.\n"); exit(1); }
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
        if (!expressionsCompatible($1, $3) && !expressionsCompatible($3, $1)) {
            fprintf(stderr, "Type Error: Equality operands must have matching types.\n"); exit(1);
        }
        $$ = createTree(0, booltype, NULL, NODE_EQ, "==", $1, NULL, $3);
    }
  | E NE E {
        if (!expressionsCompatible($1, $3) && !expressionsCompatible($3, $1)) {
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
  | NEW '(' ID ')' {
        struct Classtable *c = CLookup($3->varname);
        if (!c) { fprintf(stderr, "Semantic Error: Unknown class '%s' in new().\n", $3->varname); exit(1); }
        $$ = createTree(0, CLASS_TYPE, NULL, NODE_NEW, NULL, NULL, NULL, NULL);
        $$->Ctype = c;
    }
  | Variable { $$ = $1; }
  | FieldFunction { $$ = $1; }
  | FREE '(' Variable ')' {
        if (!IsUserType($3->type) && $3->Ctype == NULL) { fprintf(stderr, "Type Error: free() requires a user-defined type or class object.\n"); exit(1); }
        $$ = createTree(0, inttype, NULL, NODE_FREE, NULL, $3, NULL, NULL);
    }
  | ID '(' ArgList ')' {
        if (in_class_method) { fprintf(stderr, "Semantic Error: Class methods cannot call global functions.\n"); exit(1); }
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
              if (in_class_method) { fprintf(stderr, "Semantic Error: Class methods cannot access global arrays.\n"); exit(1); }
              struct Gsymbol *entry = GLookUp($1->varname);
              if (entry == NULL || entry->size <= 0 || entry->col_size != -1) {
                  fprintf(stderr, "Error: '%s' is not a one-dimensional array.\n", $1->varname); exit(1);
              }
              $$ = createTree(0, entry->type, $1->varname, NODE_ARRAY, NULL, $3, NULL, NULL);
              $$->Gentry = entry;
          }
         | ID '[' E ']' '[' E ']' {
              if (in_class_method) { fprintf(stderr, "Semantic Error: Class methods cannot access global arrays.\n"); exit(1); }
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

Field : SELF '.' ID {
            $$ = makeClassField(makeSelfNode(), $3);
        }
      | ID '.' ID {
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

FieldFunction : SELF '.' ID '(' ArgList ')' {
                    $$ = makeMethodCall(makeSelfNode(), $3, $5);
                }
              | ID '.' ID '(' ArgList ')' {
                    struct tnode *receiver = lookupVariableNode($1);
                    $$ = makeMethodCall(receiver, $3, $5);
                }
              | Field '.' ID '(' ArgList ')' {
                    $$ = makeMethodCall($1, $3, $5);
                }
              ;

%%

static struct tnode *lookupVariableNode(struct tnode *id) {
    struct Lsymbol *local = LLookUp(id->varname);
    if (local) {
        struct tnode *n = createTree(0, local->type, id->varname, NODE_ID, NULL, NULL, NULL, NULL);
        n->Lentry = local;
        n->Ctype = local->ctype;
        return n;
    }
    if (in_class_method) {
        fprintf(stderr, "Semantic Error: A class method can access only its locals, parameters, self fields, and methods. '%s' is not local.\n", id->varname);
        exit(1);
    }
    struct Gsymbol *global = GLookUp(id->varname);
    if (!global || global->size < 0 || global->col_size != -2) {
        fprintf(stderr, "Error: Undeclared variable or array requires an index: '%s'.\n", id->varname);
        exit(1);
    }
    struct tnode *n = createTree(0, global->type, id->varname, NODE_ID, NULL, NULL, NULL, NULL);
    n->Gentry = global;
    n->Ctype = global->ctype;
    return n;
}

static struct tnode *lookupArrayNode(struct tnode *id, struct tnode *index) {
    if (in_class_method) {
        fprintf(stderr, "Semantic Error: Class methods cannot access global arrays.\n");
        exit(1);
    }
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

static struct tnode *makeSelfNode(void) {
    if (!in_class_method || !current_class) {
        fprintf(stderr, "Semantic Error: 'self' is valid only inside a class method.\n");
        exit(1);
    }
    struct tnode *n = createTree(0, CLASS_TYPE, "self", NODE_SELF, NULL, NULL, NULL, NULL);
    n->Ctype = current_class;
    n->Lentry = LLookUp("self");
    return n;
}

static struct tnode *makeClassField(struct tnode *base, struct tnode *member) {
    if (!in_class_method || !base || base->Ctype == NULL || base->Ctype != current_class || base->nodetype != NODE_SELF) {
        fprintf(stderr, "Semantic Error: Class fields are private; use self.field inside a method of the same class.\n");
        exit(1);
    }
    struct Fieldlist *field = Class_Flookup(base->Ctype, member->varname);
    if (!field) {
        fprintf(stderr, "Semantic Error: Class '%s' has no member field '%s'.\n", base->Ctype->name, member->varname);
        exit(1);
    }
    struct tnode *n = createTree(field->fieldIndex, field->type, member->varname, NODE_CLASS_FIELD, NULL, base, NULL, NULL);
    n->Ctype = field->ctype;
    n->Fentry = field;
    return n;
}

static struct tnode *makeField(struct tnode *base, struct tnode *member) {
    if (!base || base->Ctype != NULL) {
        fprintf(stderr, "Semantic Error: Class fields are private; nested class field access is not supported.\n");
        exit(1);
    }
    if (!IsUserType(base->type)) {
        fprintf(stderr, "Type Error: Member access requires a user-defined record before '.%s'.\n", member->varname);
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

static int expressionsCompatible(const struct tnode *lhs, const struct tnode *rhs) {
    if (!lhs || !rhs) return 0;
    if (lhs->Ctype || rhs->Ctype) {
        if (lhs->Ctype && rhs->Ctype)
            return IsClassDescendant(rhs->Ctype, lhs->Ctype);
        if (lhs->Ctype && rhs->type == nulltype) return 1;
        if (rhs->Ctype && lhs->type == nulltype) return 1;
        return 0;
    }
    return IsCompatibleType(lhs->type, rhs->type);
}

static struct tnode *makeMethodCall(struct tnode *receiver, struct tnode *method_id, struct tnode *args) {
    if (!receiver || !receiver->Ctype) {
        fprintf(stderr, "Type Error: '%s' is not a class object; methods can be called only on class instances.\n", receiver && receiver->varname ? receiver->varname : "expression");
        exit(1);
    }
    if (in_class_method) {
        int valid_self = receiver->nodetype == NODE_SELF && receiver->Ctype == current_class;
        int valid_member = receiver->nodetype == NODE_CLASS_FIELD && receiver->left &&
                           receiver->left->nodetype == NODE_SELF && receiver->left->Ctype == current_class;
        if (!valid_self && !valid_member) {
            fprintf(stderr, "Semantic Error: A method may call only its own methods or methods of its class-valued member fields.\n");
            exit(1);
        }
    } else if (!(receiver->nodetype == NODE_ID && receiver->Gentry && receiver->Gentry->ctype)) {
        fprintf(stderr, "Semantic Error: Class objects are global-only.\n");
        exit(1);
    }
    struct Memberfunclist *method = resolveMethod(receiver->Ctype, method_id->varname, args);
    if (!method) {
        fprintf(stderr, "Semantic Error: No matching overload of method '%s' for class '%s'.\n",
                method_id->varname, receiver->Ctype->name);
        exit(1);
    }
    checkMethodArguments(method, args);
    struct tnode *call = createTree(0, method->type, method_id->varname, NODE_METHODCALL, NULL, receiver, NULL, NULL);
    call->Ctype = NULL; /* Class-valued method returns are not supported. */
    call->Mentry = method;
    call->arglist = args;
    return call;
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
    if (!expressionsCompatible(lhs, rhs)) {
        fprintf(stderr, "Type Error: Incompatible assignment to '%s'.\n", lhs->varname ? lhs->varname : "expression");
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

static struct Memberfunclist *resolveMethod(struct Classtable *ctype, const char *name, struct tnode *args) {
    struct Memberfunclist *match = NULL;
    int matches = 0;
    for (struct Memberfunclist *m = ctype ? ctype->methods : NULL; m; m = m->next) {
        if (strcmp(m->name, name) != 0) continue;
        struct Paramstruct *p = m->paramlist;
        struct tnode *a = args;
        int valid = 1;
        while (p && a) {
            if (!IsCompatibleType(p->type, a->type)) { valid = 0; break; }
            p = p->next;
            a = a->argnext;
        }
        if (p || a) valid = 0;
        if (valid) { match = m; matches++; }
    }
    if (matches > 1) {
        fprintf(stderr, "Type Error: Call to overloaded method '%s' is ambiguous; use more specific arguments.\n", name);
        exit(1);
    }
    return match;
}

static void checkMethodArguments(struct Memberfunclist *method, struct tnode *args) {
    struct Paramstruct *p = method ? method->paramlist : NULL;
    struct tnode *a = args;
    while (p && a) {
        if (!IsCompatibleType(p->type, a->type)) {
            fprintf(stderr, "Type Error: Argument type mismatch in call to method '%s'.\n", method->name);
            exit(1);
        }
        p = p->next;
        a = a->argnext;
    }
    if (p || a) {
        fprintf(stderr, "Type Error: Argument count mismatch in call to method '%s'.\n", method->name);
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
    ClassTableCreate();
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