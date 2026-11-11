int labels = 0;
int count = -1;
int nextBinding = 4096;
int nextFunctionLabel = 0;
struct Gsymbol *Ghead = NULL;
struct Gsymbol *CurrentFunction = NULL;
struct Lsymbol *Lhead = NULL;
struct Lsymbol *MainLhead = NULL;
struct tnode *MainAST = NULL;

int breakStack[1024];
int continueStack[1024];
int loopTop = -1;

/* R19 is reserved as a temporary while a function-call result is being
 * protected from restoration of the caller's saved registers. */
#define CALL_TEMP_REG 19

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

struct Gsymbol *LookupFunction(char *name)
{
    struct Gsymbol *g = Lookup(name);

    if (g != NULL && g->isFunction)
        return g;

    return NULL;
}

void Install(char *name, int baseType, int size, int isArray, int dims,
             int dim1, int dim2, int ptrDepth)
{
    if (Lookup(name) != NULL)
    {
        fprintf(stderr, "Error: '%s' is already declared\n", name);
        exit(1);
    }

    struct Gsymbol *temp = (struct Gsymbol *)malloc(sizeof(struct Gsymbol));
    if (!temp)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    temp->name = strdup(name);
    if (!temp->name)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    temp->baseType = baseType;
    temp->ptrDepth = ptrDepth;
    temp->type = makePtrType(baseType, ptrDepth);
    temp->size = size;
    temp->isArray = isArray;
    temp->dims = dims;
    temp->dim1 = dim1;
    temp->dim2 = dim2;
    temp->binding = nextBinding;
    temp->isFunction = 0;
    temp->defined = 0;
    temp->flabel = -1;
    temp->paramlist = NULL;
    temp->Lhead = NULL;
    temp->AST = NULL;
    temp->next = NULL;

    nextBinding += size;

    if (!Ghead)
        Ghead = temp;
    else
    {
        struct Gsymbol *p = Ghead;
        while (p->next)
            p = p->next;
        p->next = temp;
    }
}

void InstallFunction(char *name, int returnType, struct Paramstruct *params)
{
    if (Lookup(name) != NULL)
    {
        fprintf(stderr, "Error: '%s' is already declared\n", name);
        exit(1);
    }

    struct Gsymbol *temp = (struct Gsymbol *)malloc(sizeof(struct Gsymbol));
    if (!temp)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    temp->name = strdup(name);
    if (!temp->name)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    temp->type = returnType;
    temp->baseType = returnType;
    temp->ptrDepth = 0;
    temp->size = 0;
    temp->isArray = 0;
    temp->dims = 0;
    temp->dim1 = 1;
    temp->dim2 = 1;
    temp->binding = -1;
    temp->isFunction = 1;
    temp->defined = 0;
    temp->flabel = nextFunctionLabel++;
    temp->paramlist = params;
    temp->Lhead = NULL;
    temp->AST = NULL;
    temp->next = NULL;

    if (!Ghead)
        Ghead = temp;
    else
    {
        struct Gsymbol *p = Ghead;
        while (p->next)
            p = p->next;
        p->next = temp;
    }
}

struct Paramstruct *CreateParam(char *name, int type)
{
    struct Paramstruct *p = (struct Paramstruct *)malloc(sizeof(struct Paramstruct));
    if (!p)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    p->name = strdup(name);
    if (!p->name)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    p->type = type;
    p->next = NULL;
    return p;
}

void AppendParam(struct Paramstruct **head, struct Paramstruct *p)
{
    if (!*head)
    {
        *head = p;
        return;
    }

    struct Paramstruct *q = *head;
    while (q->next)
        q = q->next;
    q->next = p;
}

int CheckSignature(struct Gsymbol *g, int returnType, struct Paramstruct *params)
{
    if (!g || !g->isFunction)
        return 0;

    if (g->type != returnType)
        return 0;

    struct Paramstruct *a = g->paramlist;
    struct Paramstruct *b = params;

    while (a && b)
    {
        if (a->type != b->type || strcmp(a->name, b->name) != 0)
            return 0;
        a = a->next;
        b = b->next;
    }

    return a == NULL && b == NULL;
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
    struct Lsymbol *l = LookupLocal(name);
    if (l)
        return l;

    struct Gsymbol *g = Lookup(name);
    if (g && !g->isFunction)
        return NULL; /* caller must use the GST separately */

    return NULL;
}

void InstallLocal(char *name, int type, int binding)
{
    if (LookupLocal(name) != NULL)
    {
        fprintf(stderr, "Error: local variable '%s' declared more than once\n", name);
        exit(1);
    }

    struct Lsymbol *l = (struct Lsymbol *)malloc(sizeof(struct Lsymbol));
    if (!l)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    l->name = strdup(name);
    if (!l->name)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    l->type = type;
    l->binding = binding;
    l->next = NULL;

    if (!Lhead)
        Lhead = l;
    else
    {
        struct Lsymbol *p = Lhead;
        while (p->next)
            p = p->next;
        p->next = l;
    }
}

static int nextLocalBinding = 1;

void BeginFunction(struct Gsymbol *g)
{
    if (!g || !g->isFunction)
    {
        fprintf(stderr, "Error: invalid function definition\n");
        exit(1);
    }

    if (g->defined)
    {
        fprintf(stderr, "Error: function '%s' defined more than once\n", g->name);
        exit(1);
    }

    CurrentFunction = g;
    Lhead = NULL;
    nextLocalBinding = 1;

    /* Parameter 1 is at BP-3, parameter 2 at BP-4, ... */
    int paramBinding = -3;
    struct Paramstruct *p = g->paramlist;

    while (p)
    {
        InstallLocal(p->name, p->type, paramBinding);
        paramBinding--;
        p = p->next;
    }
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
        CurrentFunction->Lhead = Lhead;

    CurrentFunction = NULL;
    Lhead = NULL;
}

void PrintLocalSymbolTable(struct Lsymbol *head)
{
    struct Lsymbol *l = head;

    printf("%-15s %-10s %-8s\n", "Name", "Type", "Binding");
    printf("-----------------------------------------\n");

    while (l)
    {
        printf("%-15s %-10s %-8d\n",
               l->name,
               l->type == TYPE_INT ? "INT" : "STR",
               l->binding);
        l = l->next;
    }
}

void PrintGsymbolTable(void)
{
    struct Gsymbol *g = Ghead;

    printf("\nGlobal Symbol Table\n");
    printf("------------------------------------------------------------\n");
    printf("%-15s %-8s %-8s %-8s %-8s\n",
           "Name", "Type", "Kind", "Binding", "Label");
    printf("------------------------------------------------------------\n");

    while (g)
    {
        if (g->isFunction)
        {
            printf("%-15s %-8s %-8s %-8s F%d\n",
                   g->name,
                   g->type == TYPE_INT ? "INT" : "STR",
                   "FUNC",
                   "-",
                   g->flabel);
        }
        else
        {
            char typeName[32];
            const char *base = g->baseType == TYPE_INT ? "INT" : "STR";
            int n = snprintf(typeName, sizeof(typeName), "%s", base);

            for (int i = 0; i < g->ptrDepth && n < (int)sizeof(typeName)-2; i++)
                typeName[n++] = '*';
            typeName[n] = '\0';

            printf("%-15s %-8s %-8s %-8d %-8s\n",
                   g->name,
                   typeName,
                   "VAR",
                   g->binding,
                   "-");
        }

        g = g->next;
    }
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

void freeParamList(struct Paramstruct *head)
{
    while (head)
    {
        struct Paramstruct *next = head->next;
        free(head->name);
        free(head);
        head = next;
    }
}

void freeAST(struct tnode *t)
{
    if (!t)
        return;

    freeAST(t->left);
    freeAST(t->right);
    freeAST(t->third);

    struct Arglist *a = t->arglist;
    while (a)
    {
        struct Arglist *next = a->next;
        freeAST(a->expr);
        free(a);
        a = next;
    }

    free(t->varname);
    free(t->strval);
    free(t);
}

struct Arglist *AppendArg(struct Arglist *head, struct tnode *e)
{
    struct Arglist *node = (struct Arglist *)malloc(sizeof(struct Arglist));
    if (!node)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    node->expr = e;
    node->next = NULL;

    if (!head)
        return node;

    struct Arglist *p = head;
    while (p->next)
        p = p->next;
    p->next = node;
    return head;
}

int CountArgs(struct Arglist *a)
{
    int n = 0;
    while (a)
    {
        n++;
        a = a->next;
    }
    return n;
}

int getReg(void)
{
    /* Keep R19 free for function-call result preservation. */
    if (count < 18)
        return ++count;

    printf("Out of registers\n");
    exit(1);
}

void freeReg(void)
{
    if (count >= 0)
        count--;
}

int lab(void)
{
    return labels++;
}

static int typesCompatible(int a, int b)
{
    return a == b;
}

struct tnode *createTree(int val, int type, char *varname, char *strval,
                         int nodetype, struct Gsymbol *Gentry,
                         struct Lsymbol *Lentry, struct Arglist *arglist,
                         struct tnode *left, struct tnode *right,
                         struct tnode *third)
{
    struct tnode *n = (struct tnode *)malloc(sizeof(struct tnode));
    if (!n)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    n->val = val;
    n->type = type;
    n->nodetype = nodetype;
    n->Gentry = Gentry;
    n->Lentry = Lentry;
    n->arglist = arglist;
    n->varname = varname ? strdup(varname) : NULL;
    n->strval = strval ? strdup(strval) : NULL;
    n->left = left;
    n->right = right;
    n->third = third;

    if (nodetype == NODE_PLUS || nodetype == NODE_MINUS || nodetype == NODE_MUL ||
        nodetype == NODE_DIV || nodetype == NODE_MOD)
    {
        if (!left || !right || left->type != TYPE_INT || right->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        n->type = TYPE_INT;
    }
    else if (nodetype == NODE_LT || nodetype == NODE_GT || nodetype == NODE_LE ||
             nodetype == NODE_GE || nodetype == NODE_EQ || nodetype == NODE_NE)
    {
        if (!left || !right || left->type != TYPE_INT || right->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        n->type = TYPE_BOOL;
    }
    else if (nodetype == NODE_ASSGN)
    {
        if (!left || !right || !typesCompatible(left->type, right->type))
        {
            printf("Type Mismatch in assignment\n");
            exit(1);
        }
        n->type = left->type;
    }
    else if (nodetype == NODE_READ)
    {
        if (!left || left->type != TYPE_INT)
        {
            printf("Type Mismatch: read expects an integer lvalue\n");
            exit(1);
        }
        n->type = -1;
    }
    else if (nodetype == NODE_WRITE)
    {
        if (!left || (left->type != TYPE_INT && left->type != TYPE_STR))
        {
            printf("Type Mismatch in write\n");
            exit(1);
        }
        n->type = -1;
    }
    else if (nodetype == NODE_IF || nodetype == NODE_WHILE)
    {
        if (!left || left->type != TYPE_BOOL)
        {
            printf("Type Mismatch: condition must be boolean\n");
            exit(1);
        }
        n->type = -1;
    }
    else if (nodetype == NODE_NUM)
        n->type = TYPE_INT;
    else if (nodetype == NODE_STR)
        n->type = TYPE_STR;
    else if (nodetype == NODE_ID)
    {
        if (!Gentry && !Lentry)
        {
            printf("Undeclared variable\n");
            exit(1);
        }
        n->type = Lentry ? Lentry->type : Gentry->type;
    }
    else if (nodetype == NODE_ARRAY)
    {
        if (!Gentry || !Gentry->isArray)
        {
            printf("Invalid array reference\n");
            exit(1);
        }

        if (!left || left->type != TYPE_INT)
        {
            printf("Type Mismatch: array index must be integer\n");
            exit(1);
        }

        if (right != NULL)
        {
            if (Gentry->dims != 2 || right->type != TYPE_INT)
            {
                printf("Invalid two-dimensional array reference\n");
                exit(1);
            }
        }
        else if (Gentry->dims != 1)
        {
            printf("Error: two-dimensional array requires two indices\n");
            exit(1);
        }

        n->type = Gentry->type;
    }
    else if (nodetype == NODE_ADDR)
    {
        if (!left)
            n->type = type;
        else
            n->type = makePtrType(baseTypeOf(left->type), pointerDepth(left->type) + 1);
    }
    else if (nodetype == NODE_DEREF)
    {
        if (!left || !isPointerType(left->type))
        {
            printf("Error: dereference requires a pointer operand\n");
            exit(1);
        }

        int d = pointerDepth(left->type);
        n->type = makePtrType(baseTypeOf(left->type), d - 1);
    }
    else if (nodetype == NODE_FUNCALL)
    {
        if (!Gentry || !Gentry->isFunction)
        {
            printf("Invalid function call\n");
            exit(1);
        }
        n->type = Gentry->type;
    }
    else if (nodetype == NODE_RETURN)
    {
        n->type = -1;
    }
    else if (nodetype == NODE_BREAK || nodetype == NODE_CONTINUE || nodetype == NODE_CONNECTOR)
        n->type = -1;

    return n;
}

/* Resolve an ordinary variable in the current function scope. */
static void resolveVariable(char *name, struct Gsymbol **g, struct Lsymbol **l)
{
    *g = NULL;
    *l = LookupLocal(name);

    if (*l)
        return;

    *g = Lookup(name);

    if (!*g || (*g)->isFunction)
    {
        fprintf(stderr, "Error: variable '%s' is not declared\n", name);
        exit(1);
    }
}

int expr(struct tnode *t, FILE *f);

/* Emit the address of any supported lvalue. */
static int lvalueAddr(struct tnode *t, FILE *f)
{
    int addr, row, col, temp;

    if (t->nodetype == NODE_ID)
    {
        if (t->Lentry)
        {
            addr = getReg();
            fprintf(f, "MOV R%d, BP\n", addr);
            fprintf(f, "ADD R%d, %d\n", addr, t->Lentry->binding);
            return addr;
        }

        addr = getReg();
        fprintf(f, "MOV R%d, %d\n", addr, t->Gentry->binding);
        return addr;
    }

    if (t->nodetype == NODE_DEREF)
        return expr(t->left, f);

    if (t->nodetype != NODE_ARRAY)
    {
        printf("Invalid lvalue\n");
        exit(1);
    }

    addr = getReg();
    fprintf(f, "MOV R%d, %d\n", addr, t->Gentry->binding);

    row = expr(t->left, f);

    if (t->right == NULL)
    {
        fprintf(f, "ADD R%d, R%d\n", addr, row);
        freeReg();
        return addr;
    }

    col = expr(t->right, f);
    temp = getReg();

    fprintf(f, "MOV R%d, %d\n", temp, t->Gentry->dim2);
    fprintf(f, "MUL R%d, R%d\n", row, temp);
    fprintf(f, "ADD R%d, R%d\n", addr, row);
    fprintf(f, "ADD R%d, R%d\n", addr, col);

    freeReg();
    freeReg();
    freeReg();
    return addr;
}

static void pushArgsReverse(struct Arglist *args, FILE *f)
{
    if (!args)
        return;

    if (args->next)
        pushArgsReverse(args->next, f);

    int r = expr(args->expr, f);
    fprintf(f, "PUSH R%d\n", r);
    freeReg();
}

static int generateFunctionCall(struct tnode *t, FILE *f)
{
    int savedCount = count;
    int argCount = CountArgs(t->arglist);
    int resultReg;
    int i;

    /* Caller saves every register currently in use. */
    for (i = 0; i <= savedCount; i++)
        fprintf(f, "PUSH R%d\n", i);

    count = -1;

    /* Push arg_n ... arg_2 arg_1 so that arg_1 becomes BP-3. */
    pushArgsReverse(t->arglist, f);

    /* One empty stack slot for the callee's return value. */
    fprintf(f, "PUSH R0\n");

    fprintf(f, "CALL F%d\n", t->Gentry->flabel);

    /* Return value is at the top of the stack after RET. */
    fprintf(f, "MOV R%d, [SP]\n", CALL_TEMP_REG);
    fprintf(f, "POP R0\n");

    /* Remove the arguments. */
    for (i = 0; i < argCount; i++)
        fprintf(f, "POP R0\n");

    /* Restore the caller's register context. */
    for (i = savedCount; i >= 0; i--)
        fprintf(f, "POP R%d\n", i);

    count = savedCount;

    /* R19 is reserved for the call result. If all ordinary registers up to
       R18 were live before the call, R19 itself becomes the result register. */
    if (savedCount == 18)
    {
        resultReg = CALL_TEMP_REG;
        count = CALL_TEMP_REG;
    }
    else
    {
        resultReg = getReg();
        fprintf(f, "MOV R%d, R%d\n", resultReg, CALL_TEMP_REG);
    }

    return resultReg;
}

int expr(struct tnode *t, FILE *f)
{
    int a, b;
    const char *op = "";

    if (!t)
    {
        fprintf(stderr, "Internal error: NULL expression node\n");
        exit(1);
    }

    if (t->nodetype == NODE_NUM)
    {
        a = getReg();
        fprintf(f, "MOV R%d, %d\n", a, t->val);
        return a;
    }

    if (t->nodetype == NODE_STR)
    {
        a = getReg();
        fprintf(f, "MOV R%d, \"%s\"\n", a, t->strval);
        return a;
    }

    if (t->nodetype == NODE_ID)
    {
        a = getReg();

        if (t->Lentry)
        {
            int addr = getReg();
            fprintf(f, "MOV R%d, BP\n", addr);
            fprintf(f, "ADD R%d, %d\n", addr, t->Lentry->binding);
            fprintf(f, "MOV R%d, [R%d]\n", a, addr);
            freeReg();
        }
        else
        {
            fprintf(f, "MOV R%d, [%d]\n", a, t->Gentry->binding);
        }

        return a;
    }

    if (t->nodetype == NODE_ADDR)
    {
        if (t->Gentry != NULL)
        {
            a = getReg();
            fprintf(f, "MOV R%d, %d\n", a, t->Gentry->binding);
            return a;
        }

        if (t->Lentry != NULL)
        {
            a = getReg();
            fprintf(f, "MOV R%d, BP\n", a);
            fprintf(f, "ADD R%d, %d\n", a, t->Lentry->binding);
            return a;
        }

        return lvalueAddr(t->left, f);
    }

    if (t->nodetype == NODE_DEREF)
    {
        a = expr(t->left, f);
        fprintf(f, "MOV R%d, [R%d]\n", a, a);
        return a;
    }

    if (t->nodetype == NODE_ARRAY)
    {
        a = lvalueAddr(t, f);
        fprintf(f, "MOV R%d, [R%d]\n", a, a);
        return a;
    }

    if (t->nodetype == NODE_FUNCALL)
        return generateFunctionCall(t, f);

    a = expr(t->left, f);
    b = expr(t->right, f);

    switch (t->nodetype)
    {
    case NODE_PLUS:  op = "ADD"; break;
    case NODE_MINUS: op = "SUB"; break;
    case NODE_MUL:   op = "MUL"; break;
    case NODE_DIV:   op = "DIV"; break;
    case NODE_MOD:   op = "MOD"; break;
    case NODE_LT:    op = "LT";  break;
    case NODE_GT:    op = "GT";  break;
    case NODE_LE:    op = "LE";  break;
    case NODE_GE:    op = "GE";  break;
    case NODE_EQ:    op = "EQ";  break;
    case NODE_NE:    op = "NE";  break;
    default:
        fprintf(stderr, "Invalid expression node\n");
        exit(1);
    }

    fprintf(f, "%s R%d, R%d\n", op, a, b);
    freeReg();
    return a;
}

static void emitLocalAllocation(struct Lsymbol *head, FILE *f)
{
    fprintf(f, "MOV R0, 0\n");

    while (head)
    {
        if (head->binding > 0)
            fprintf(f, "PUSH R0\n");
        head = head->next;
    }
}

static int countLocals(struct Lsymbol *head)
{
    int n = 0;
    while (head)
    {
        if (head->binding > 0)
            n++;
        head = head->next;
    }
    return n;
}

static void emitMainExit(FILE *f)
{
    fprintf(f,
            "MOV R1, \"Exit\"\n"
            "PUSH R1\n"
            "MOV R1, -1\n"
            "PUSH R1\n"
            "PUSH R1\n"
            "PUSH R1\n"
            "PUSH R1\n"
            "CALL 0\n");
}

static int generatingMain = 0;
static struct Gsymbol *generatingFunction = NULL;

void gen(struct tnode *t, FILE *f)
{
    if (!t)
        return;

    int r, q, l1, l2;

    switch (t->nodetype)
    {
    case NODE_CONNECTOR:
        gen(t->left, f);
        gen(t->right, f);
        break;

    case NODE_ASSGN:
        r = expr(t->right, f);

        if (t->left->nodetype == NODE_ID)
        {
            int addr = lvalueAddr(t->left, f);
            fprintf(f, "MOV [R%d], R%d\n", addr, r);
            freeReg();
            freeReg();
        }
        else if (t->left->nodetype == NODE_DEREF)
        {
            int addr = expr(t->left->left, f);
            fprintf(f, "MOV [R%d], R%d\n", addr, r);
            freeReg();
            freeReg();
        }
        else if (t->left->nodetype == NODE_ARRAY)
        {
            int addr = lvalueAddr(t->left, f);
            fprintf(f, "MOV [R%d], R%d\n", addr, r);
            freeReg();
            freeReg();
        }
        else
        {
            fprintf(stderr, "Invalid assignment target\n");
            exit(1);
        }
        break;

    case NODE_READ:
    {
        int status = count, i;

        for (i = 0; i <= status; i++)
            fprintf(f, "PUSH R%d\n", i);

        fprintf(f, "MOV R0, \"Read\"\nPUSH R0\n");
        fprintf(f, "MOV R0, -1\nPUSH R0\n");

        int addr = lvalueAddr(t->left, f);
        fprintf(f, "MOV R0, R%d\n", addr);
        freeReg();

        fprintf(f, "PUSH R0\nPUSH R0\nPUSH R0\nCALL 0\n");

        for (i = 0; i < 5; i++)
            fprintf(f, "POP R0\n");
        for (i = status; i >= 0; i--)
            fprintf(f, "POP R%d\n", i);

        count = status;
        break;
    }

    case NODE_WRITE:
    {
        int status = count, i;

        for (i = 0; i <= status; i++)
            fprintf(f, "PUSH R%d\n", i);

        count = -1;
        r = expr(t->left, f);
        q = getReg();
        fprintf(f, "MOV R%d, \"Write\"\nPUSH R%d\n", q, q);
        fprintf(f, "MOV R%d, -2\nPUSH R%d\n", q, q);
        fprintf(f, "PUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\n", r, q, q);

        for (i = 0; i < 5; i++)
            fprintf(f, "POP R0\n");
        for (i = status; i >= 0; i--)
            fprintf(f, "POP R%d\n", i);

        count = status;
        break;
    }

    case NODE_IF:
        r = expr(t->left, f);
        l1 = lab();
        l2 = lab();
        fprintf(f, "JZ R%d, L%d\n", r, l1);
        freeReg();
        gen(t->right, f);

        if (t->third)
        {
            fprintf(f, "JMP L%d\nL%d:\n", l2, l1);
            gen(t->third, f);
            fprintf(f, "L%d:\n", l2);
        }
        else
            fprintf(f, "L%d:\n", l1);
        break;

    case NODE_WHILE:
        l1 = lab();
        l2 = lab();
        fprintf(f, "L%d:\n", l1);
        r = expr(t->left, f);
        fprintf(f, "JZ R%d, L%d\n", r, l2);
        freeReg();

        if (loopTop >= 1023)
        {
            fprintf(stderr, "Too many nested loops\n");
            exit(1);
        }

        loopTop++;
        breakStack[loopTop] = l2;
        continueStack[loopTop] = l1;
        gen(t->right, f);
        loopTop--;

        fprintf(f, "JMP L%d\nL%d:\n", l1, l2);
        break;

    case NODE_BREAK:
        if (loopTop >= 0)
            fprintf(f, "JMP L%d\n", breakStack[loopTop]);
        break;

    case NODE_CONTINUE:
        if (loopTop >= 0)
            fprintf(f, "JMP L%d\n", continueStack[loopTop]);
        break;

    case NODE_RETURN:
        r = expr(t->left, f);

        if (generatingMain)
        {
            /* main has no caller; its return statement terminates the program. */
            freeReg();
            emitMainExit(f);
        }
        else
        {
            /* Store the return value in the caller-created slot at [BP-2]. */
            fprintf(f, "MOV R%d, BP\n", CALL_TEMP_REG);
            fprintf(f, "ADD R%d, -2\n", CALL_TEMP_REG);
            fprintf(f, "MOV [R%d], R%d\n", CALL_TEMP_REG, r);
            freeReg();

            /* Remove local variables. */
            int locals = countLocals(generatingFunction->Lhead);
            for (int i = 0; i < locals; i++)
                fprintf(f, "POP R0\n");

            /* Restore caller BP and then RET to the saved return address. */
            fprintf(f, "POP BP\n");
            fprintf(f, "RET\n");
        }
        break;

    default:
        fprintf(stderr, "Invalid statement node %d\n", t->nodetype);
        exit(1);
    }
}

void codeGenFunction(struct Gsymbol *g, FILE *f)
{
    generatingFunction = g;
    generatingMain = 0;
    count = -1;
    loopTop = -1;

    fprintf(f, "F%d:\n", g->flabel);

    /* Callee prologue. */
    fprintf(f, "PUSH BP\n");
    fprintf(f, "MOV BP, SP\n");
    emitLocalAllocation(g->Lhead, f);

    gen(g->AST, f);
}

void codeGenMain(struct tnode *ast, struct Lsymbol *locals, FILE *f)
{
    generatingFunction = NULL;
    generatingMain = 1;
    count = -1;
    loopTop = -1;

    fprintf(f, "MAIN:\n");

    /* Global variables occupy the static part of the stack. Main's activation
       record starts immediately above them. */
    int stackBase = nextBinding - 1;
    if (stackBase < 4095)
        stackBase = 4095;
    fprintf(f, "MOV SP, %d\n", stackBase);
    fprintf(f, "MOV BP, SP\n");
    emitLocalAllocation(locals, f);

    gen(ast, f);
}

void emit_exit(FILE *f)
{
    emitMainExit(f);
}