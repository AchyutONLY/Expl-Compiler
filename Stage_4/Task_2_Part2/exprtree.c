int labels = 0;
int count = -1;
int nextBinding = 4096;
struct Gsymbol *Ghead = NULL;

/* Stack of labels for the currently active while loops.
   For each loop:
   break    -> exit label
   continue -> condition label
*/
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

void Install(char *name, int type, int size, int isArray)
{
    struct Gsymbol *temp;

    if (Lookup(name) != NULL)
    {
        fprintf(stderr,
                "Error: variable '%s' declared more than once\n",
                name);
        exit(1);
    }

    temp = (struct Gsymbol *)malloc(sizeof(struct Gsymbol));

    if (temp == NULL)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(1);
    }

    temp->name = strdup(name);
    temp->type = type;
    temp->size = size;
    temp->isArray = isArray;
    temp->binding = nextBinding;
    temp->next = NULL;

    nextBinding += size;

    if (Ghead == NULL)
    {
        Ghead = temp;
    }
    else
    {
        struct Gsymbol *p = Ghead;

        while (p->next != NULL)
            p = p->next;

        p->next = temp;
    }
}

void PrintGsymbolTable(void)
{
    struct Gsymbol *temp = Ghead;

    printf("\nGlobal Symbol Table\n");
    printf("%-15s %-10s %-10s %-10s\n",
           "Name", "Type", "Size", "Binding");
    printf("----------------------------------------------------------\n");

    while (temp != NULL)
    {
        const char *typeName;

        if (temp->type == TYPE_INT)
            typeName = "INT";
        else
            typeName = "STR";

        printf("%-15s %-10s %-10d %-10d\n",
               temp->name,
               typeName,
               temp->size,
               temp->binding);

        temp = temp->next;
    }
}

int getReg()
{
    if (count < 19)
    {
        count++;
        return count;
    }

    printf("Out of registers\n");
    exit(1);
}

void freeReg()
{
    if (count >= 0)
        count--;
}

int lab()
{
    return labels++;
}

/*
 * Find the highest static memory address occupied by a declared
 * global variable. Every variable in this stage has size 1, but
 * using binding + size - 1 keeps the helper consistent with the
 * symbol-table representation.
 */
int maxVarAddr(struct tnode *t)
{
    int max = 4095;

    if (t == NULL)
        return max;

    if (t->Gentry != NULL)
    {
        int a = t->Gentry->binding + t->Gentry->size - 1;

        if (a > max)
            max = a;
    }

    {
        int x = maxVarAddr(t->left);
        int y = maxVarAddr(t->right);
        int z = maxVarAddr(t->third);

        if (x > max)
            max = x;

        if (y > max)
            max = y;

        if (z > max)
            max = z;
    }

    return max;
}

struct tnode *createTree(
    int val,
    int type,
    char *varname,
    char *strval,
    int nodetype,
    struct Gsymbol *Gentry,
    struct tnode *left,
    struct tnode *right,
    struct tnode *third)
{
    struct tnode *newnode =
        (struct tnode *)malloc(sizeof(struct tnode));

    if (newnode == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    newnode->val = val;
    newnode->type = type;
    newnode->nodetype = nodetype;
    newnode->Gentry = Gentry;

    if (varname != NULL)
    {
        newnode->varname = strdup(varname);

        if (newnode->varname == NULL)
        {
            printf("Memory allocation failed\n");
            exit(1);
        }
    }
    else
    {
        newnode->varname = NULL;
    }

    if (strval != NULL)
    {
        newnode->strval = strdup(strval);

        if (newnode->strval == NULL)
        {
            printf("Memory allocation failed\n");
            exit(1);
        }
    }
    else
    {
        newnode->strval = NULL;
    }

    if (nodetype == NODE_PLUS ||
        nodetype == NODE_MINUS ||
        nodetype == NODE_MUL ||
        nodetype == NODE_DIV ||
        nodetype == NODE_MOD)
    {
        if (left == NULL || right == NULL ||
            left->type != TYPE_INT ||
            right->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }

        newnode->type = TYPE_INT;
    }
    else if (nodetype == NODE_LT ||
             nodetype == NODE_GT ||
             nodetype == NODE_LE ||
             nodetype == NODE_GE ||
             nodetype == NODE_EQ ||
             nodetype == NODE_NE)
    {
        if (left == NULL || right == NULL ||
            left->type != TYPE_INT ||
            right->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }

        newnode->type = TYPE_BOOL;
    }
    else if (nodetype == NODE_ASSGN)
    {
        if (left == NULL || right == NULL ||
            left->type != right->type)
        {
            printf("Type Mismatch in assignment\n");
            exit(1);
        }

        newnode->type = left->type;
    }
    else if (nodetype == NODE_READ)
    {
        if (left == NULL || left->type != TYPE_INT)
        {
            printf("Type Mismatch: read expects an integer variable\n");
            exit(1);
        }

        newnode->type = -1;
    }
    else if (nodetype == NODE_WRITE)
    {
        if (left == NULL ||
            (left->type != TYPE_INT && left->type != TYPE_STR))
        {
            printf("Type Mismatch in write\n");
            exit(1);
        }

        newnode->type = -1;
    }
    else if (nodetype == NODE_IF ||
             nodetype == NODE_WHILE)
    {
        if (left == NULL || left->type != TYPE_BOOL)
        {
            printf("Type Mismatch: condition must be boolean\n");
            exit(1);
        }

        newnode->type = -1;
    }
    else if (nodetype == NODE_NUM)
    {
        newnode->type = TYPE_INT;
    }
    else if (nodetype == NODE_STR)
    {
        newnode->type = TYPE_STR;
    }
    else if (nodetype == NODE_ID)
    {
        if (Gentry == NULL)
        {
            printf("Undeclared variable\n");
            exit(1);
        }

        newnode->type = Gentry->type;
    }
    else if (nodetype == NODE_ARRAY)
    {
        if (Gentry == NULL || !Gentry->isArray)
        {
            printf("Invalid array reference\n");
            exit(1);
        }

        if (left == NULL || left->type != TYPE_INT)
        {
            printf("Type Mismatch: array index must be integer\n");
            exit(1);
        }

        newnode->type = Gentry->type;
    }
    else if (nodetype == NODE_BREAK ||
             nodetype == NODE_CONTINUE ||
             nodetype == NODE_CONNECTOR)
    {
        newnode->type = -1;
    }

    newnode->left = left;
    newnode->right = right;
    newnode->third = third;

    return newnode;
}

int expr(struct tnode *t, FILE *f)
{
    int a, b;
    const char *op = "";

    if (t->nodetype == NODE_NUM)
    {
        a = getReg();

        fprintf(f,
                "MOV R%d, %d\n",
                a,
                t->val);

        return a;
    }

    if (t->nodetype == NODE_STR)
    {
        a = getReg();

        fprintf(f,
                "MOV R%d, \"%s\"\n",
                a,
                t->strval);

        return a;
    }

    if (t->nodetype == NODE_ID)
    {
        a = getReg();

        fprintf(f,
                "MOV R%d, [%d]\n",
                a,
                t->Gentry->binding);

        return a;
    }

    if (t->nodetype == NODE_ARRAY)
    {
        /*
         * Evaluate the index, form binding + index, then load
         * the value. The index register is reused for the value.
         */
        a = expr(t->left, f);
        b = getReg();

        fprintf(f, "MOV R%d, %d\n", b, t->Gentry->binding);
        fprintf(f, "ADD R%d, R%d\n", b, a);
        fprintf(f, "MOV R%d, [R%d]\n", a, b);

        freeReg(); /* release address register b */
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

    fprintf(f,
            "%s R%d, R%d\n",
            op,
            a,
            b);

    /* Result remains in R[a], R[b] is no longer needed. */
    freeReg();

    return a;
}

void gen(struct tnode *t, FILE *f)
{
    if (t == NULL)
        return;

    int r, q, l1, l2;

    switch (t->nodetype)
    {
    case NODE_CONNECTOR:
    {
        gen(t->left, f);
        gen(t->right, f);
        break;
    }

    case NODE_ASSGN:
    {
        r = expr(t->right, f);

        if (t->left->nodetype == NODE_ID)
        {
            fprintf(
                f,
                "MOV [%d], R%d\n",
                t->left->Gentry->binding,
                r);

            freeReg();
        }
        else if (t->left->nodetype == NODE_ARRAY)
        {
            /*
             * Keep the RHS live while calculating the array
             * element address.
             */
            int indexReg = expr(t->left->left, f);
            int addrReg = getReg();

            fprintf(f,
                    "MOV R%d, %d\n",
                    addrReg,
                    t->left->Gentry->binding);
            fprintf(f,
                    "ADD R%d, R%d\n",
                    addrReg,
                    indexReg);
            fprintf(f,
                    "MOV [R%d], R%d\n",
                    addrReg,
                    r);

            freeReg(); /* address register */
            freeReg(); /* index register */
            freeReg(); /* RHS register */
        }
        else
        {
            printf("Invalid assignment target\n");
            exit(1);
        }

        break;
    }

    case NODE_READ:
    {
        int status = count;
        int i;

        /*
         * Preserve every register currently live.
         */
        for (i = 0; i <= status; i++)
            fprintf(f, "PUSH R%d\n", i);

        /*
         * Use R0 for the library call setup.
         * The five stack words are:
         *
         *   function name
         *   fd = -1
         *   variable binding
         *   blank argument
         *   return value slot
         */
        fprintf(f, "MOV R0, \"Read\"\n");
        fprintf(f, "PUSH R0\n");

        fprintf(f, "MOV R0, -1\n");
        fprintf(f, "PUSH R0\n");

        if (t->left->nodetype == NODE_ID)
        {
            fprintf(f,
                    "MOV R0, %d\n",
                    t->left->Gentry->binding);
        }
        else if (t->left->nodetype == NODE_ARRAY)
        {
            int indexReg = expr(t->left->left, f);
            int addrReg = getReg();

            fprintf(f,
                    "MOV R%d, %d\n",
                    addrReg,
                    t->left->Gentry->binding);

            fprintf(f,
                    "ADD R%d, R%d\n",
                    addrReg,
                    indexReg);

            fprintf(f,
                    "MOV R0, R%d\n",
                    addrReg);

            freeReg(); /* address register */
            freeReg(); /* index register */
        }
        else
        {
            printf("Invalid read target\n");
            exit(1);
        }

        fprintf(f, "PUSH R0\n");

        fprintf(f, "PUSH R0\n");
        fprintf(f, "PUSH R0\n");

        fprintf(f, "CALL 0\n");

        /*
         * Remove the five library-call stack entries.
         */
        fprintf(f, "POP R0\n");
        fprintf(f, "POP R0\n");
        fprintf(f, "POP R0\n");
        fprintf(f, "POP R0\n");
        fprintf(f, "POP R0\n");

        /*
         * Restore all registers that were live before Read.
         */
        for (i = status; i >= 0; i--)
            fprintf(f, "POP R%d\n", i);

        count = status;
        break;
    }

    case NODE_WRITE:
    {
        int status = count;
        int i;

        /*
         * Preserve every register currently live.
         */
        for (i = 0; i <= status; i++)
            fprintf(f, "PUSH R%d\n", i);

        /*
         * Start temporary expression allocation from R0,
         * because all previously live registers are already saved.
         */
        count = -1;

        r = expr(t->left, f);

        /*
         * Use R1 for the library call descriptor, leaving
         * the value to print safely in R0.
         */
        q = getReg();

        fprintf(f, "MOV R%d, \"Write\"\n", q);
        fprintf(f, "PUSH R%d\n", q);

        fprintf(f, "MOV R%d, -2\n", q);
        fprintf(f, "PUSH R%d\n", q);

        fprintf(f, "PUSH R%d\n", r);

        /* Third argument: blank */
        fprintf(f, "PUSH R%d\n", q);

        /* Storage for return value */
        fprintf(f, "PUSH R%d\n", q);

        fprintf(f, "CALL 0\n");

        /*
         * Remove the five library-call stack entries.
         */
        fprintf(f, "POP R1\n");
        fprintf(f, "POP R1\n");
        fprintf(f, "POP R1\n");
        fprintf(f, "POP R1\n");
        fprintf(f, "POP R1\n");

        /*
         * Restore all registers live before Write.
         */
        for (i = status; i >= 0; i--)
            fprintf(f, "POP R%d\n", i);

        count = status;
        break;
    }

    case NODE_IF:
    {
        r = expr(t->left, f);

        l1 = lab();
        l2 = lab();

        fprintf(f,
                "JZ R%d, L%d\n",
                r,
                l1);

        freeReg();

        gen(t->right, f);

        if (t->third != NULL)
        {
            fprintf(f,
                    "JMP L%d\n",
                    l2);

            fprintf(f,
                    "L%d:\n",
                    l1);

            gen(t->third, f);

            fprintf(f,
                    "L%d:\n",
                    l2);
        }
        else
        {
            fprintf(f,
                    "L%d:\n",
                    l1);
        }

        break;
    }

    case NODE_WHILE:
    {
        /*
         * l1 = condition label
         * l2 = exit label
         */
        l1 = lab();
        l2 = lab();

        fprintf(f,
                "L%d:\n",
                l1);

        r = expr(t->left, f);

        fprintf(f,
                "JZ R%d, L%d\n",
                r,
                l2);

        freeReg();

        if (loopTop >= 1023)
        {
            printf("Too many nested loops\n");
            exit(1);
        }

        /*
         * This loop becomes the innermost active loop.
         */
        loopTop++;

        breakStack[loopTop] = l2;
        continueStack[loopTop] = l1;

        gen(t->right, f);

        /*
         * Restore the enclosing loop, if any.
         */
        loopTop--;

        fprintf(f,
                "JMP L%d\n",
                l1);

        fprintf(f,
                "L%d:\n",
                l2);

        break;
    }

    case NODE_BREAK:
    {
        /*
         * Jump to the exit label of the innermost loop.
         */
        if (loopTop >= 0)
        {
            fprintf(f,
                    "JMP L%d\n",
                    breakStack[loopTop]);
        }

        break;
    }

    case NODE_CONTINUE:
    {
        /*
         * Jump to the condition label of the innermost loop.
         */
        if (loopTop >= 0)
        {
            fprintf(f,
                    "JMP L%d\n",
                    continueStack[loopTop]);
        }

        break;
    }

    default:
        printf("Invalid statement node\n");
        exit(1);
    }
}

void codeGen(struct tnode *t, FILE *f)
{
    /*
     * Variables are allocated statically starting at 4096.
     * nextBinding points to the first free location after all
     * declarations, so nextBinding - 1 is the last occupied
     * variable address. PUSH increments SP before storing.
     */
    int stackBase = nextBinding - 1;

    /*
     * If there are no variables, stackBase remains 4095.
     * This makes the first PUSH land at 4096.
     */
    if (stackBase < 4095)
        stackBase = 4095;

    fprintf(f,
            "MOV SP, %d\n",
            stackBase);

    /*
     * Register allocator / loop state should start fresh.
     */
    count = -1;
    loopTop = -1;

    gen(t, f);
}

void emit_exit(FILE *f)
{
    fprintf(
        f,
        "MOV R1, \"Exit\"\n"
        "PUSH R1\n"
        "MOV R1, -1\n"
        "PUSH R1\n"
        "PUSH R1\n"
        "PUSH R1\n"
        "PUSH R1\n"
        "CALL 0\n");
}