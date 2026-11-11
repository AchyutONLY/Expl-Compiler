#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BOOL 0
#define TYPE_INT 1
#define TYPE_STR 2

/* Pointer types from Stage 4. */
#define TYPE_PTR_INT 3
#define TYPE_PTR_STR 4

/* Tuple types for Stage 5 Exercise 2. */
#define TYPE_TUPLE_BASE 100000
#define TYPE_TUPLE_PTR_BASE 200000

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
#define NODE_FUNCALL 26
#define NODE_RETURN 27
#define NODE_AND 28
#define NODE_OR 29
#define NODE_NOT 30
#define NODE_MEMBER 31


struct TupleField
{
    char *name;
    int type;
    int offset;
    struct TupleField *next;
};

struct TupleType
{
    char *name;
    int id;
    int size;
    struct TupleField *fields;
    struct TupleType *next;
};

struct TupleVar
{
    char *name;
    int isPointer;
    struct TupleVar *next;
};

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
    int binding;
    int size;
    int isTuple;
    int isTuplePtr;
    struct TupleType *tupleType;
    struct Lsymbol *next;
};

struct Gsymbol
{
    char *name;
    int type;          /* encoded type */
    int baseType;      /* TYPE_INT / TYPE_STR */
    int ptrDepth;      /* Stage 4 globals */
    int size;
    int isArray;
    int dims;
    int dim1;
    int dim2;
    int binding;      /* static binding for global variables; -1 for functions */

    int isFunction;
    int defined;
    int flabel;
    struct Paramstruct *paramlist;
    struct Lsymbol *Lhead;
    struct tnode *AST;

    int isTuple;
    int isTuplePtr;
    struct TupleType *tupleType;

    struct Gsymbol *next;
};

struct Arglist
{
    struct tnode *expr;
    struct Arglist *next;
};

struct tnode
{
    int val;
    int type;
    char *varname;
    char *strval;
    int nodetype;

    struct Gsymbol *Gentry; /* used for global variables and functions */
    struct Lsymbol *Lentry; /* used for local variables / parameters */
    struct Arglist *arglist; /* used by NODE_FUNCALL */

    struct tnode *left, *right, *third;
};

extern struct Gsymbol *Ghead;
extern struct Gsymbol *CurrentFunction;
extern struct Lsymbol *Lhead;
extern struct Lsymbol *MainLhead;
extern struct tnode *MainAST;
extern int nextBinding;

static inline int isTupleType(int type)
{
    return type >= TYPE_TUPLE_BASE && type < TYPE_TUPLE_PTR_BASE;
}

static inline int isTuplePointerType(int type)
{
    return type >= TYPE_TUPLE_PTR_BASE;
}

static inline int tupleIdOfType(int type)
{
    if (isTupleType(type))
        return type - TYPE_TUPLE_BASE;

    if (isTuplePointerType(type))
        return (type - TYPE_TUPLE_PTR_BASE) / 1000;

    return -1;
}

static inline int makeTupleType(int tupleId)
{
    return TYPE_TUPLE_BASE + tupleId;
}

static inline int makeTuplePtrType(int tupleId, int depth)
{
    if (depth <= 0)
        return makeTupleType(tupleId);

    return TYPE_TUPLE_PTR_BASE + tupleId * 1000 + depth;
}

static inline int makePtrType(int baseType, int depth)
{
    if (depth <= 0)
        return baseType;

    if (isTupleType(baseType))
        return makeTuplePtrType(tupleIdOfType(baseType), depth);

    return (baseType == TYPE_INT)
               ? TYPE_PTR_INT + 2 * (depth - 1)
               : TYPE_PTR_STR + 2 * (depth - 1);
}

static inline int isPointerType(int type)
{
    if (isTuplePointerType(type))
        return 1;

    return type >= TYPE_PTR_INT && type < TYPE_TUPLE_BASE;
}

static inline int pointerDepth(int type)
{
    if (isTuplePointerType(type))
        return (type - TYPE_TUPLE_PTR_BASE) % 1000;

    if (!isPointerType(type))
        return 0;

    if (type & 1)
        return (type - TYPE_PTR_INT) / 2 + 1;

    return (type - TYPE_PTR_STR) / 2 + 1;
}

static inline int baseTypeOf(int type)
{
    if (isTuplePointerType(type))
        return makeTupleType(tupleIdOfType(type));

    if (isTupleType(type) || !isPointerType(type))
        return type;

    return (type & 1) ? TYPE_INT : TYPE_STR;
}

struct TupleType *LookupTupleType(char *);
struct TupleType *DefineTupleType(char *, struct TupleField *);
struct TupleType *GetOrDefineTupleType(char *, struct TupleField *);
struct TupleField *LookupTupleField(struct TupleType *, char *);
void PrintTupleTypes(void);
struct tnode *MakeTupleMemberNode(char *, char *);

struct Gsymbol *Lookup(char *);
struct Gsymbol *LookupFunction(char *);
void Install(char *, int, int, int, int, int, int, int);
void InstallFunction(char *, int, struct Paramstruct *);
void InstallTupleGlobal(char *, struct TupleType *, int);
void PrintGsymbolTable(void);

struct Paramstruct *CreateParam(char *, int);
void AppendParam(struct Paramstruct **, struct Paramstruct *);
int CheckSignature(struct Gsymbol *, int, struct Paramstruct *);

struct Lsymbol *LookupLocal(char *);
struct Lsymbol *LookupVariable(char *);
void InstallLocal(char *, int, int);
void InstallTupleLocal(char *, struct TupleType *, int, int);
int NextLocalBinding(int);
void BeginFunction(struct Gsymbol *);
void BeginMain(void);
void EndFunction(void);
void PrintLocalSymbolTable(struct Lsymbol *);
void freeLocalSymbolTable(struct Lsymbol *);
void freeParamList(struct Paramstruct *);
void freeAST(struct tnode *);

struct Arglist *AppendArg(struct Arglist *, struct tnode *);
int CountArgs(struct Arglist *);

struct tnode *createTree(int, int, char *, char *, int, struct Gsymbol *,
                         struct Lsymbol *, struct Arglist *,
                         struct tnode *, struct tnode *, struct tnode *);

void codeGenFunction(struct Gsymbol *, FILE *);
void codeGenMain(struct tnode *, struct Lsymbol *, FILE *);
void emit_exit(FILE *);