int labels = 0;
int count = -1;
int nextBinding = 4096;
struct Gsymbol *Ghead = NULL;

int breakStack[1024];
int continueStack[1024];
int loopTop = -1;

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
        fprintf(stderr, "Error: variable '%s' declared more than once\n", name);
        exit(1);
    }

    struct Gsymbol *temp = (struct Gsymbol *)malloc(sizeof(struct Gsymbol));
    if (!temp)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    temp->name = strdup(name);
    temp->baseType = baseType;
    temp->ptrDepth = ptrDepth;
    temp->type = makePtrType(baseType, ptrDepth);
    temp->size = size;
    temp->isArray = isArray;
    temp->dims = dims;
    temp->dim1 = dim1;
    temp->dim2 = dim2;
    temp->binding = nextBinding;
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

void PrintGsymbolTable(void)
{
    struct Gsymbol *temp = Ghead;
    printf("\nGlobal Symbol Table\n");
    printf("%-15s %-10s %-8s %-8s %-8s %-12s\n",
           "Name", "Type", "PtrDepth", "Size", "Binding", "Dimensions");
    printf("---------------------------------------------------------------------\n");

    while (temp)
    {
        const char *base = (temp->baseType == TYPE_INT) ? "INT" : "STR";
        char typeName[32];
        if (temp->ptrDepth == 0)
            snprintf(typeName, sizeof(typeName), "%s", base);
        else
        {
            int n = snprintf(typeName, sizeof(typeName), "%s", base);
            for (int i = 0; i < temp->ptrDepth && n < (int)sizeof(typeName) - 2; i++)
                typeName[n++] = '*';
            typeName[n] = '\0';
        }

        if (temp->dims == 2)
            printf("%-15s %-10s %-8d %-8d %-8d %dx%d\n",
                   temp->name, typeName, temp->ptrDepth, temp->size,
                   temp->binding, temp->dim1, temp->dim2);
        else
            printf("%-15s %-10s %-8d %-8d %-8d %-12s\n",
                   temp->name, typeName, temp->ptrDepth, temp->size,
                   temp->binding, temp->isArray ? "1D" : "scalar");
        temp = temp->next;
    }
}

int getReg()
{
    if (count < 19)
        return ++count;
    printf("Out of registers\n");
    exit(1);
}

void freeReg()
{
    if (count >= 0)
        count--;
}

int lab() { return labels++; }

int maxVarAddr(struct tnode *t)
{
    if (!t)
        return 4095;
    int max = 4095;
    if (t->Gentry)
    {
        int a = t->Gentry->binding + t->Gentry->size - 1;
        if (a > max)
            max = a;
    }
    int x = maxVarAddr(t->left), y = maxVarAddr(t->right), z = maxVarAddr(t->third);
    if (x > max)
        max = x;
    if (y > max)
        max = y;
    if (z > max)
        max = z;
    return max;
}

static int typesCompatible(int a, int b) { return a == b; }

struct tnode *createTree(int val, int type, char *varname, char *strval,
                         int nodetype, struct Gsymbol *Gentry,
                         struct tnode *left, struct tnode *right,
                         struct tnode *third)
{
    struct tnode *n = (struct tnode *)malloc(sizeof(struct tnode));
    if (!n)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    n->val = val;
    n->type = type;
    n->nodetype = nodetype;
    n->Gentry = Gentry;
    n->varname = varname ? strdup(varname) : NULL;
    n->strval = strval ? strdup(strval) : NULL;

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
        if (!Gentry)
        {
            printf("Undeclared variable\n");
            exit(1);
        }
        n->type = Gentry->type;
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
            if (Gentry->dims != 2)
            {
                printf("Error: one-dimensional array indexed with two indices\n");
                exit(1);
            }
            if (right->type != TYPE_INT)
            {
                printf("Type Mismatch: array index must be integer\n");
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
            n->type = type; /* scalar &ID stores Gentry directly */
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
    else if (nodetype == NODE_BREAK || nodetype == NODE_CONTINUE || nodetype == NODE_CONNECTOR)
        n->type = -1;

    n->left = left;
    n->right = right;
    n->third = third;
    return n;
}

int expr(struct tnode *t, FILE *f);

/* Emit the address of any supported lvalue and return its register. */
static int lvalueAddr(struct tnode *t, FILE *f)
{
    int addr, row, col, temp;

    if (t->nodetype == NODE_ID)
    {
        addr = getReg();
        fprintf(f, "MOV R%d, %d\n", addr, t->Gentry->binding);
        return addr;
    }

    if (t->nodetype == NODE_DEREF)
    {
        /* The pointer value itself is the address of the lvalue. */
        return expr(t->left, f);
    }

    if (t->nodetype != NODE_ARRAY)
    {
        printf("Invalid lvalue\n");
        exit(1);
    }

    /* Allocate the address register first, so it remains the returned
       register after temporary index registers are released. */
    addr = getReg();
    fprintf(f, "MOV R%d, %d\n", addr, t->Gentry->binding);

    row = expr(t->left, f);

    if (t->right == NULL)
    {
        fprintf(f, "ADD R%d, R%d\n", addr, row);
        freeReg(); /* row */
        return addr;
    }

    col = expr(t->right, f);
    temp = getReg();

    fprintf(f, "MOV R%d, %d\n", temp, t->Gentry->dim2);
    fprintf(f, "MUL R%d, R%d\n", row, temp);
    fprintf(f, "ADD R%d, R%d\n", addr, row);
    fprintf(f, "ADD R%d, R%d\n", addr, col);

    freeReg(); /* temp */
    freeReg(); /* col */
    freeReg(); /* row */
    return addr;
}

int expr(struct tnode *t, FILE *f)
{
    int a, b;
    const char *op = "";

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
        fprintf(f, "MOV R%d, [%d]\n", a, t->Gentry->binding);
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
        a = lvalueAddr(t->left, f);
        return a;
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

    a = expr(t->left, f);
    b = expr(t->right, f);

    switch (t->nodetype)
    {
    case NODE_PLUS:
        op = "ADD";
        break;
    case NODE_MINUS:
        op = "SUB";
        break;
    case NODE_MUL:
        op = "MUL";
        break;
    case NODE_DIV:
        op = "DIV";
        break;
    case NODE_MOD:
        op = "MOD";
        break;
    case NODE_LT:
        op = "LT";
        break;
    case NODE_GT:
        op = "GT";
        break;
    case NODE_LE:
        op = "LE";
        break;
    case NODE_GE:
        op = "GE";
        break;
    case NODE_EQ:
        op = "EQ";
        break;
    case NODE_NE:
        op = "NE";
        break;
    default:
        printf("Invalid expression node\n");
        exit(1);
    }

    fprintf(f, "%s R%d, R%d\n", op, a, b);
    freeReg();
    return a;
}

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
            fprintf(f, "MOV [%d], R%d\n", t->left->Gentry->binding, r);
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
            printf("Invalid assignment target\n");
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
            fprintf(f, "POP R1\n");
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
            printf("Too many nested loops\n");
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
    default:
        printf("Invalid statement node\n");
        exit(1);
    }
}

void codeGen(struct tnode *t, FILE *f)
{
    int stackBase = nextBinding - 1;
    if (stackBase < 4095)
        stackBase = 4095;
    fprintf(f, "MOV SP, %d\n", stackBase);
    count = -1;
    loopTop = -1;
    gen(t, f);
}

void emit_exit(FILE *f)
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