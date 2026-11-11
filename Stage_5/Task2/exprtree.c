#include "exprtree.h"

int nextBinding = 4096;
int nextFLabel = 0;
int nextLocalBinding = 1;
struct Gsymbol *Ghead = NULL;
struct Gsymbol *CurrentFunction = NULL;
struct Lsymbol *Lhead = NULL;

static int isSimpleType(int type)
{
    return type == TYPE_INT || type == TYPE_STR;
}

static void semanticError(const char *msg)
{
    fprintf(stderr, "Semantic error: %s\n", msg);
    exit(1);
}

static const char *typeName(int type)
{
    switch (type)
    {
    case TYPE_INT:
        return "INT";
    case TYPE_STR:
        return "STR";
    case TYPE_BOOL:
        return "BOOL";
    default:
        break;
    }

    return "PTR";
}

/* ----------------------------- GST ----------------------------- */

struct Gsymbol *Lookup(char *name)
{
    struct Gsymbol *temp = Ghead;

    while (temp != NULL)
    {
        if (strcmp(temp->name, name) == 0)
            return temp;
        temp = temp->next;
    }

    return NULL;
}

void Install(char *name, int baseType, int size, int isArray, int dims,
             int dim1, int dim2, int ptrDepth)
{
    if (Lookup(name) != NULL)
    {
        fprintf(stderr, "Semantic error: '%s' is already declared\n", name);
        exit(1);
    }

    struct Gsymbol *entry = (struct Gsymbol *)calloc(1, sizeof(struct Gsymbol));
    if (!entry)
        semanticError("memory allocation failed");

    entry->name = strdup(name);
    entry->baseType = baseType;
    entry->ptrDepth = ptrDepth;
    entry->type = makePtrType(baseType, ptrDepth);
    entry->size = size;
    entry->isArray = isArray;
    entry->dims = dims;
    entry->dim1 = dim1;
    entry->dim2 = dim2;
    entry->binding = nextBinding;
    entry->paramlist = NULL;
    entry->flabel = -1;
    entry->isFunction = 0;
    entry->defined = 0;
    entry->Lhead = NULL;
    entry->AST = NULL;

    nextBinding += size;

    if (!Ghead)
        Ghead = entry;
    else
    {
        struct Gsymbol *p = Ghead;
        while (p->next)
            p = p->next;
        p->next = entry;
    }
}

void InstallFunction(char *name, int returnType, struct Paramstruct *params)
{
    if (Lookup(name) != NULL)
    {
        fprintf(stderr, "Semantic error: '%s' is already declared\n", name);
        exit(1);
    }

    if (!isSimpleType(returnType))
        semanticError("function return type must be int or str at Stage 5");

    struct Gsymbol *entry = (struct Gsymbol *)calloc(1, sizeof(struct Gsymbol));
    if (!entry)
        semanticError("memory allocation failed");

    entry->name = strdup(name);
    entry->type = returnType;
    entry->baseType = returnType;
    entry->ptrDepth = 0;
    entry->size = 0;
    entry->isArray = 0;
    entry->dims = 0;
    entry->dim1 = 1;
    entry->dim2 = 1;
    entry->binding = -1;     /* Functions do not consume static variable bindings. */
    entry->paramlist = params;
    entry->flabel = nextFLabel++;
    entry->isFunction = 1;
    entry->defined = 0;
    entry->Lhead = NULL;
    entry->AST = NULL;

    if (!Ghead)
        Ghead = entry;
    else
    {
        struct Gsymbol *p = Ghead;
        while (p->next)
            p = p->next;
        p->next = entry;
    }
}

void printParamList(struct Paramstruct *p)
{
    if (!p)
    {
        printf("- ");
        return;
    }

    while (p)
    {
        printf("%s %s", typeName(p->type), p->name);
        if (p->next)
            printf(", ");
        p = p->next;
    }
}

void PrintGsymbolTable(void)
{
    struct Gsymbol *g = Ghead;

    printf("\n================ GLOBAL SYMBOL TABLE ================\n");
    printf("%-14s %-8s %-8s %-8s %-10s %-18s %-8s\n",
           "NAME", "TYPE", "KIND", "SIZE", "BINDING", "PARAMETERS", "FLABEL");
    printf("--------------------------------------------------------------------------\n");

    while (g)
    {
        if (g->isFunction)
        {
            printf("%-14s %-8s %-8s %-8s %-10s ",
                   g->name, typeName(g->type), "FUNCTION", "-", "-");
            printParamList(g->paramlist);
            printf(" %-8d\n", g->flabel);
        }
        else
        {
            printf("%-14s %-8s %-8s %-8d %-10d %-18s %-8s\n",
                   g->name,
                   typeName(g->type),
                   "VARIABLE",
                   g->size,
                   g->binding,
                   g->isArray ? "array" : "-",
                   "-");
        }

        g = g->next;
    }

    printf("======================================================\n");
}

void CheckAllFunctionsDefined(void)
{
    struct Gsymbol *g = Ghead;

    while (g)
    {
        if (g->isFunction && !g->defined)
        {
            fprintf(stderr, "Semantic error: function '%s' was declared but not defined\n", g->name);
            exit(1);
        }
        g = g->next;
    }
}

/* -------------------------- Parameters -------------------------- */

struct Paramstruct *createParam(char *name, int type)
{
    struct Paramstruct *p = (struct Paramstruct *)calloc(1, sizeof(struct Paramstruct));
    if (!p)
        semanticError("memory allocation failed");

    p->name = strdup(name);
    p->type = type;
    p->next = NULL;
    return p;
}

void appendParam(struct Paramstruct **head, struct Paramstruct *p)
{
    if (!*head)
    {
        *head = p;
        return;
    }

    struct Paramstruct *q = *head;
    while (q->next)
    {
        if (strcmp(q->name, p->name) == 0)
        {
            fprintf(stderr, "Semantic error: parameter '%s' declared more than once\n", p->name);
            exit(1);
        }
        q = q->next;
    }

    if (strcmp(q->name, p->name) == 0)
    {
        fprintf(stderr, "Semantic error: parameter '%s' declared more than once\n", p->name);
        exit(1);
    }

    q->next = p;
}

int compareParams(struct Paramstruct *decl, struct Paramstruct *def, char *functionName)
{
    struct Paramstruct *p = decl;
    struct Paramstruct *q = def;
    int pos = 1;

    while (p && q)
    {
        if (p->type != q->type)
        {
            fprintf(stderr,
                    "Semantic error: parameter %d of '%s' has type %s in declaration but %s in definition\n",
                    pos, functionName, typeName(p->type), typeName(q->type));
            exit(1);
        }

        if (strcmp(p->name, q->name) != 0)
        {
            fprintf(stderr,
                    "Semantic error: parameter %d of '%s' is named '%s' in declaration but '%s' in definition\n",
                    pos, functionName, p->name, q->name);
            exit(1);
        }

        p = p->next;
        q = q->next;
        pos++;
    }

    if (p || q)
    {
        fprintf(stderr,
                "Semantic error: number of parameters of '%s' does not match its declaration\n",
                functionName);
        exit(1);
    }

    return 1;
}

void freeParamList(struct Paramstruct *p)
{
    while (p)
    {
        struct Paramstruct *next = p->next;
        free(p->name);
        free(p);
        p = next;
    }
}

/* ---------------------------- LST ---------------------------- */

void BeginFunction(struct Gsymbol *g)
{
    if (!g || !g->isFunction)
        semanticError("invalid function being defined");

    if (g->defined)
    {
        fprintf(stderr, "Semantic error: function '%s' is defined more than once\n", g->name);
        exit(1);
    }

    CurrentFunction = g;
    Lhead = NULL;
    nextLocalBinding = 1;

    /* Install formal parameters first, preserving declaration order. */
    struct Paramstruct *p = g->paramlist;
    while (p)
    {
        InstallLocal(p->name, p->type);
        p = p->next;
    }

    g->Lhead = Lhead;
}

void BeginMain(void)
{
    CurrentFunction = NULL;
    Lhead = NULL;
    nextLocalBinding = 1;
}

void EndFunction(void)
{
    if (CurrentFunction)
    {
        CurrentFunction->Lhead = Lhead;
    }

    CurrentFunction = NULL;
    Lhead = NULL;
}

struct Lsymbol *LookupLocal(char *name)
{
    struct Lsymbol *l = Lhead;

    while (l)
    {
        if (strcmp(l->name, name) == 0)
            return l;
        l = l->next;
    }

    return NULL;
}

struct Lsymbol *LookupVariable(char *name)
{
    struct Lsymbol *local = LookupLocal(name);
    if (local)
        return local;

    return NULL;
}

void InstallLocal(char *name, int type)
{
    if (!isSimpleType(type))
        semanticError("only int and str locals are allowed at Stage 5");

    if (LookupLocal(name))
    {
        fprintf(stderr, "Semantic error: local variable '%s' declared more than once\n", name);
        exit(1);
    }

    struct Lsymbol *entry = (struct Lsymbol *)calloc(1, sizeof(struct Lsymbol));
    if (!entry)
        semanticError("memory allocation failed");

    entry->name = strdup(name);
    entry->type = type;
    entry->binding = nextLocalBinding++; /* Relative binding for Task 2. */
    entry->next = NULL;

    if (!Lhead)
        Lhead = entry;
    else
    {
        struct Lsymbol *p = Lhead;
        while (p->next)
            p = p->next;
        p->next = entry;
    }

    if (CurrentFunction)
        CurrentFunction->Lhead = Lhead;
}

void freeLocalSymbolTable(struct Lsymbol *head)
{
    while (head)
    {
        struct Lsymbol *next = head->next;
        free(head->name);
        free(head);
        head = next;
    }
}

void PrintLocalSymbolTable(struct Lsymbol *head)
{
    printf("LOCAL SYMBOL TABLE\n");
    printf("%-14s %-8s %-8s\n", "NAME", "TYPE", "BINDING");
    printf("--------------------------------\n");

    while (head)
    {
        printf("%-14s %-8s %-8d\n", head->name, typeName(head->type), head->binding);
        head = head->next;
    }
    printf("--------------------------------\n");
}

/* ----------------------------- AST ----------------------------- */

struct tnode *createTree(int val, int type, char *varname, char *strval,
                         int nodetype, struct Gsymbol *Gentry,
                         struct Lsymbol *Lentry,
                         struct tnode *arglist,
                         struct tnode *left,
                         struct tnode *right,
                         struct tnode *third,
                         struct tnode *next)
{
    struct tnode *n = (struct tnode *)calloc(1, sizeof(struct tnode));
    if (!n)
        semanticError("memory allocation failed");

    n->val = val;
    n->type = type;
    n->nodetype = nodetype;
    n->Gentry = Gentry;
    n->Lentry = Lentry;
    n->varname = varname ? strdup(varname) : NULL;
    n->strval = strval ? strdup(strval) : NULL;
    n->arglist = arglist;
    n->left = left;
    n->right = right;
    n->third = third;
    n->next = next;

    switch (nodetype)
    {
    case NODE_NUM:
        n->type = TYPE_INT;
        break;

    case NODE_STR:
        n->type = TYPE_STR;
        break;

    case NODE_ID:
        if (!Gentry && !Lentry)
            semanticError("identifier node has no symbol-table entry");
        n->type = Lentry ? Lentry->type : Gentry->type;
        break;

    case NODE_PLUS:
    case NODE_MINUS:
    case NODE_MUL:
    case NODE_DIV:
    case NODE_MOD:
        if (!left || !right || left->type != TYPE_INT || right->type != TYPE_INT)
            semanticError("arithmetic operands must be int");
        n->type = TYPE_INT;
        break;

    case NODE_LT:
    case NODE_GT:
    case NODE_LE:
    case NODE_GE:
        if (!left || !right || left->type != TYPE_INT || right->type != TYPE_INT)
            semanticError("relational operands must be int");
        n->type = TYPE_BOOL;
        break;

    case NODE_EQ:
    case NODE_NE:
        if (!left || !right || left->type != right->type)
            semanticError("operands of == and != must have the same type");
        n->type = TYPE_BOOL;
        break;

    case NODE_ASSGN:
        if (!left || !right || left->type != right->type)
            semanticError("type mismatch in assignment");
        n->type = -1;
        break;

    case NODE_READ:
        if (!left || (left->type != TYPE_INT && left->type != TYPE_STR))
            semanticError("read expects int or str lvalue");
        n->type = -1;
        break;

    case NODE_WRITE:
        if (!left || (left->type != TYPE_INT && left->type != TYPE_STR))
            semanticError("write expects int or str expression");
        n->type = -1;
        break;

    case NODE_IF:
    case NODE_WHILE:
        if (!left || left->type != TYPE_BOOL)
            semanticError("condition must be boolean");
        n->type = -1;
        break;

    case NODE_ARRAY:
        if (!Gentry || !Gentry->isArray)
            semanticError("invalid array reference");
        if (!left || left->type != TYPE_INT)
            semanticError("array index must be int");
        if (right && right->type != TYPE_INT)
            semanticError("array index must be int");
        n->type = Gentry->type;
        break;

    case NODE_ADDR:
        if (left)
            n->type = makePtrType(baseTypeOf(left->type), pointerDepth(left->type) + 1);
        else
            n->type = type;
        break;

    case NODE_DEREF:
        if (!left || !isPointerType(left->type))
            semanticError("dereference requires pointer operand");
        n->type = makePtrType(baseTypeOf(left->type), pointerDepth(left->type) - 1);
        break;

    case NODE_RET:
        n->type = type;
        if (!left)
            semanticError("return expression missing");
        if (left->type != type)
            semanticError("return expression type does not match function return type");
        break;

    case NODE_BODY:
        n->type = type;
        break;

    case NODE_MAIN:
        n->type = TYPE_INT;
        break;

    case NODE_CONNECTOR:
    case NODE_BREAK:
    case NODE_CONTINUE:
        n->type = -1;
        break;

    case NODE_CALL:
        n->type = type;
        break;

    default:
        break;
    }

    return n;
}

static int countArgs(struct tnode *args)
{
    int count = 0;
    while (args)
    {
        count++;
        args = args->next;
    }
    return count;
}

struct tnode *appendArg(struct tnode *head, struct tnode *arg)
{
    if (!head)
        return arg;

    struct tnode *p = head;
    while (p->next)
        p = p->next;
    p->next = arg;
    return head;
}

struct tnode *createCallTree(struct Gsymbol *function, struct tnode *args)
{
    if (!function || !function->isFunction)
        semanticError("called identifier is not a function");

    struct Paramstruct *p = function->paramlist;
    struct tnode *a = args;
    int pos = 1;

    while (p && a)
    {
        if (p->type != a->type)
        {
            fprintf(stderr,
                    "Semantic error: argument %d of '%s' has type %s but expected %s\n",
                    pos, function->name, typeName(a->type), typeName(p->type));
            exit(1);
        }
        p = p->next;
        a = a->next;
        pos++;
    }

    if (p || a)
    {
        fprintf(stderr,
                "Semantic error: number of arguments in call to '%s' does not match its declaration\n",
                function->name);
        exit(1);
    }

    (void)countArgs;

    return createTree(0, function->type, function->name, NULL,
                      NODE_CALL, function, NULL, args, NULL, NULL, NULL, NULL);
}

static const char *nodeName(int nodetype)
{
    switch (nodetype)
    {
    case NODE_NUM: return "NUM";
    case NODE_STR: return "STRCONST";
    case NODE_ID: return "ID";
    case NODE_PLUS: return "PLUS";
    case NODE_MINUS: return "MINUS";
    case NODE_MUL: return "MUL";
    case NODE_DIV: return "DIV";
    case NODE_MOD: return "MOD";
    case NODE_LT: return "LT";
    case NODE_GT: return "GT";
    case NODE_LE: return "LE";
    case NODE_GE: return "GE";
    case NODE_EQ: return "EQ";
    case NODE_NE: return "NE";
    case NODE_ASSGN: return "ASSIGN";
    case NODE_READ: return "READ";
    case NODE_WRITE: return "WRITE";
    case NODE_IF: return "IF";
    case NODE_WHILE: return "WHILE";
    case NODE_CONNECTOR: return "SLIST";
    case NODE_BREAK: return "BREAK";
    case NODE_CONTINUE: return "CONTINUE";
    case NODE_ARRAY: return "ARRAY";
    case NODE_ADDR: return "ADDR";
    case NODE_DEREF: return "DEREF";
    case NODE_RET: return "RETURN";
    case NODE_CALL: return "FUNCTION_CALL";
    case NODE_BODY: return "BODY";
    case NODE_MAIN: return "MAIN";
    default: return "UNKNOWN";
    }
}

static void printASTInternal(struct tnode *t, int depth)
{
    if (!t)
        return;

    for (int i = 0; i < depth; i++)
        printf("  ");

    printf("%s", nodeName(t->nodetype));

    if (t->nodetype == NODE_NUM)
        printf(" value=%d", t->val);
    else if (t->nodetype == NODE_STR)
        printf(" value=\"%s\"", t->strval ? t->strval : "");
    else if (t->varname)
        printf(" name=%s", t->varname);

    if (t->Lentry)
        printf(" [local:%s]", t->Lentry->name);
    else if (t->Gentry)
        printf(" [global:%s]", t->Gentry->name);

    printf(" type=%s\n", typeName(t->type));

    if (t->arglist)
    {
        for (int i = 0; i < depth + 1; i++)
            printf("  ");
        printf("ARGS\n");

        struct tnode *a = t->arglist;
        while (a)
        {
            printASTInternal(a, depth + 2);
            a = a->next;
        }
    }

    if (t->left)
        printASTInternal(t->left, depth + 1);
    if (t->right)
        printASTInternal(t->right, depth + 1);
    if (t->third)
        printASTInternal(t->third, depth + 1);
}

void printAST(struct tnode *t, int depth)
{
    printASTInternal(t, depth);
}

void freeAST(struct tnode *t)
{
    if (!t)
        return;

    freeAST(t->left);
    freeAST(t->right);
    freeAST(t->third);

    if (t->arglist)
    {
        struct tnode *a = t->arglist;
        while (a)
        {
            struct tnode *next = a->next;
            a->next = NULL;
            freeAST(a);
            a = next;
        }
    }

    free(t->varname);
    free(t->strval);
    free(t);
}

/* Task 2 deliberately stops before code generation. */
void codeGen(struct tnode *t, FILE *f)
{
    (void)t;
    (void)f;
}

void emit_exit(FILE *f)
{
    (void)f;
}