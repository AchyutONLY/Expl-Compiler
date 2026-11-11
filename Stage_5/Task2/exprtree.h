#ifndef EXPRTREE_H
#define EXPRTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BOOL 0
#define TYPE_INT 1
#define TYPE_STR 2

/* Stage 4 pointer encoding retained for compatibility with the supplied code. */
#define TYPE_PTR_INT 3
#define TYPE_PTR_STR 4

#define NODE_NUM 1
#define NODE_ID 2
#define NODE_PLUS 3
#define NODE_MINUS 4
#define NODE_MUL 5
#define NODE_DIV 6
#define NODE_MOD 7
#define NODE_LT 8
#define NODE_GT 9
#define NODE_LE 10
#define NODE_GE 11
#define NODE_EQ 12
#define NODE_NE 13
#define NODE_ASSGN 14
#define NODE_READ 15
#define NODE_WRITE 16
#define NODE_IF 17
#define NODE_WHILE 18
#define NODE_CONNECTOR 19
#define NODE_BREAK 20
#define NODE_CONTINUE 21
#define NODE_STR 22
#define NODE_ARRAY 23
#define NODE_ADDR 24
#define NODE_DEREF 25

/* Stage 5 Task 2 nodes. */
#define NODE_RET 26
#define NODE_CALL 27
#define NODE_BODY 28
#define NODE_MAIN 29
#define NODE_ARGLIST 30

struct Paramstruct
{
    char *name;
    int type;
    struct Paramstruct *next;
};

struct Lsymbol
{
    char *name;
    int type;
    int binding;                 /* Relative binding; exact layout is for later stages. */
    struct Lsymbol *next;
};

struct tnode;

struct Gsymbol
{
    char *name;
    int type;
    int baseType;
    int ptrDepth;
    int size;
    int isArray;
    int dims;
    int dim1;
    int dim2;
    int binding;

    /* Stage 5 fields. */
    struct Paramstruct *paramlist;
    int flabel;
    int isFunction;
    int defined;
    struct Lsymbol *Lhead;
    struct tnode *AST;

    struct Gsymbol *next;
};

struct tnode
{
    int val;
    int type;
    char *varname;
    char *strval;
    int nodetype;

    struct Gsymbol *Gentry;
    struct Lsymbol *Lentry;

    /* Function-call argument list and ordinary AST links. */
    struct tnode *arglist;
    struct tnode *next;
    struct tnode *left;
    struct tnode *right;
    struct tnode *third;
};

extern struct Gsymbol *Ghead;
extern struct Gsymbol *CurrentFunction;
extern struct Lsymbol *Lhead;
extern int nextBinding;
extern int nextFLabel;
extern int nextLocalBinding;

static inline int makePtrType(int baseType, int depth)
{
    if (depth <= 0)
        return baseType;

    return (baseType == TYPE_INT)
               ? TYPE_PTR_INT + 2 * (depth - 1)
               : TYPE_PTR_STR + 2 * (depth - 1);
}

static inline int isPointerType(int type)
{
    return type >= TYPE_PTR_INT;
}

static inline int pointerDepth(int type)
{
    if (type < TYPE_PTR_INT)
        return 0;

    if (type & 1)
        return (type - TYPE_PTR_INT) / 2 + 1;

    return (type - TYPE_PTR_STR) / 2 + 1;
}

static inline int baseTypeOf(int type)
{
    if (type < TYPE_PTR_INT)
        return type;

    return (type & 1) ? TYPE_INT : TYPE_STR;
}

/* Global symbol table. */
struct Gsymbol *Lookup(char *name);
void Install(char *, int, int, int, int, int, int, int);
void InstallFunction(char *, int, struct Paramstruct *);
void PrintGsymbolTable(void);
void CheckAllFunctionsDefined(void);

/* Parameter lists. */
struct Paramstruct *createParam(char *, int);
void appendParam(struct Paramstruct **, struct Paramstruct *);
int compareParams(struct Paramstruct *, struct Paramstruct *, char *);
void printParamList(struct Paramstruct *);
void freeParamList(struct Paramstruct *);

/* Local symbol table. */
void BeginFunction(struct Gsymbol *);
void BeginMain(void);
void EndFunction(void);
struct Lsymbol *LookupLocal(char *);
struct Lsymbol *LookupVariable(char *);
void InstallLocal(char *, int);
void PrintLocalSymbolTable(struct Lsymbol *);
void freeLocalSymbolTable(struct Lsymbol *);

/* AST. */
struct tnode *createTree(int, int, char *, char *, int, struct Gsymbol *,
                         struct Lsymbol *, struct tnode *, struct tnode *,
                         struct tnode *, struct tnode *, struct tnode *);

struct tnode *createCallTree(struct Gsymbol *, struct tnode *);
struct tnode *appendArg(struct tnode *, struct tnode *);
void printAST(struct tnode *, int);
void freeAST(struct tnode *);

/* Kept as compatibility stubs for the supplied Stage 4 codebase. */
void codeGen(struct tnode *, FILE *);
void emit_exit(FILE *);

#endif