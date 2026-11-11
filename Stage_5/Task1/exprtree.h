#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BOOL 0
#define TYPE_INT 1
#define TYPE_STR 2

/* Pointer types are encoded by pointer depth and base type.
 * int* = 3, str* = 4, int** = 5, str** = 6, ... */
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

/* Formal parameter information for a function declaration. */
struct Paramstruct
{
    char *name;
    int type;
    struct Paramstruct *next;
};

struct Gsymbol
{
    char *name;
    int type;          /* return type for a function, variable type otherwise */
    int baseType;      /* TYPE_INT or TYPE_STR */
    int ptrDepth;      /* 0 for ordinary variable, >=1 for pointer */
    int size;
    int isArray;
    int dims;
    int dim1;
    int dim2;
    int binding;       /* static binding for global variables */
    struct Paramstruct *paramlist; /* formal parameters for functions */
    int flabel;        /* pseudo label of the function; -1 for variables */
    int isFunction;    /* 1 for function, 0 for variable */
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
    struct tnode *left, *right, *third;
};

extern struct Gsymbol *Ghead;
extern int nextBinding;

/* Return encoded pointer type for base type + depth. */
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

struct Gsymbol *Lookup(char *);
void Install(char *, int, int, int, int, int, int, int);
void InstallFunction(char *, int, struct Paramstruct *);
struct Paramstruct *createParam(char *, int);
void appendParam(struct Paramstruct **, struct Paramstruct *);
void PrintGsymbolTable(void);

struct tnode *createTree(int, int, char *, char *, int, struct Gsymbol *,
                         struct tnode *, struct tnode *, struct tnode *);

void codeGen(struct tnode *, FILE *);
void emit_exit(FILE *);