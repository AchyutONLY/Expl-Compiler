int labels = 0;
int count = -1;

/* Stack of labels for the currently active while loops.
   For each loop:
   break    -> exit label
   continue -> condition label
*/
int breakStack[1024];
int continueStack[1024];
int loopTop = -1;

int getReg()
{
    if (count < 20)
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

int addr(char *s)
{
    return 4096 + s[0] - 'a';
}

/*
 * Find the highest memory address used by a variable in the AST.
 *
 * Variables are stored starting from address 4096:
 *   a -> 4096
 *   b -> 4097
 *   c -> 4098
 *   ...
 *
 * The stack pointer must be initialized to the highest variable
 * address. Since PUSH increments SP before storing, the first PUSH
 * will then use the next free address.
 */
int maxVarAddr(struct tnode *t)
{
    int max = 4095;

    if (t == NULL)
        return max;

    if (t->nodetype == NODE_ID && t->varname != NULL)
    {
        int a = addr(t->varname);
        if (a > max)
            max = a;
    }

    if (t->nodetype == NODE_ASSGN &&
        t->left != NULL &&
        t->left->nodetype == NODE_ID &&
        t->left->varname != NULL)
    {
        int a = addr(t->left->varname);
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
    int nodetype,
    struct tnode *left,
    struct tnode *right,
    struct tnode *third
)
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

    if (varname != NULL)
    {
        newnode->varname =
            (char *)malloc(strlen(varname) + 1);

        if (newnode->varname == NULL)
        {
            printf("Memory allocation failed\n");
            exit(1);
        }

        strcpy(newnode->varname, varname);
    }
    else
    {
        newnode->varname = NULL;
    }

    newnode->nodetype = nodetype;

    if (nodetype == NODE_PLUS ||
        nodetype == NODE_MINUS ||
        nodetype == NODE_MUL ||
        nodetype == NODE_DIV ||
        nodetype == NODE_MOD)
    {
        if (left->type != TYPE_INT ||
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
        if (left->type != TYPE_INT ||
            right->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }

        newnode->type = TYPE_BOOL;
    }
    else if (nodetype == NODE_ASSGN)
    {
        if (left->type != TYPE_INT ||
            right->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }

        newnode->type = TYPE_INT;
    }
    else if (nodetype == NODE_READ ||
             nodetype == NODE_WRITE)
    {
        if (left->type != TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }

        newnode->type = -1;
    }
    else if (nodetype == NODE_IF ||
             nodetype == NODE_WHILE ||
             nodetype == NODE_DO_WHILE ||
             nodetype == NODE_REPEAT_UNTIL)
    {
        if (left->type != TYPE_BOOL)
        {
            printf("Type Mismatch\n");
            exit(1);
        }

        newnode->type = -1;
    }
    else if (nodetype == NODE_NUM ||
             nodetype == NODE_ID)
    {
        newnode->type = TYPE_INT;
    }

    newnode->left = left;
    newnode->right = right;
    newnode->third = third;

    return newnode;
}

int expr(struct tnode *t, FILE *f)
{
    int a, b;

    if (t->nodetype == NODE_NUM)
    {
        a = getReg();
        fprintf(f, "MOV R%d, %d\n", a, t->val);
        return a;
    }

    if (t->nodetype == NODE_ID)
    {
        a = getReg();
        fprintf(
            f,
            "MOV R%d, [%d]\n",
            a,
            addr(t->varname)
        );
        return a;
    }

    a = expr(t->left, f);
    b = expr(t->right, f);

    const char *op = "";

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
    }

    fprintf(f, "%s R%d, R%d\n", op, a, b);

    /* Result remains in R[a], R[b] is no longer needed. */
    freeReg();

    return a;
}

void gen(struct tnode *t, FILE *f)
{
    if (t == NULL)
        return;

    int r, l1, l2, l3;

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

            fprintf(
                f,
                "MOV [%d], R%d\n",
                addr(t->left->varname),
                r
            );

            freeReg();
            break;
        }

        case NODE_READ:
        {
            /*
             * Save all currently live registers.
             */
            for (int i = 0; i <= count; i++)
                fprintf(f, "PUSH R%d\n", i);

            int status = count;

            /*
             * ExpL/XSM Read ABI:
             *
             *   function name = "Read"
             *   arg1          = -1
             *   arg2          = variable address
             *   arg3          = dummy
             *   return slot   = dummy
             */
            fprintf(f, "MOV R0, \"Read\"\n");
            fprintf(f, "PUSH R0\n");

            fprintf(f, "MOV R0, -1\n");
            fprintf(f, "PUSH R0\n");

            fprintf(f, "MOV R0, %d\n", addr(t->left->varname));
            fprintf(f, "PUSH R0\n");

            fprintf(f, "PUSH R0\n");
            fprintf(f, "PUSH R0\n");

            fprintf(f, "CALL 0\n");

            /*
             * Remove the 5 values passed to CALL 0.
             */
            fprintf(f, "POP R0\n");
            fprintf(f, "POP R1\n");
            fprintf(f, "POP R1\n");
            fprintf(f, "POP R1\n");
            fprintf(f, "POP R1\n");

            /*
             * Restore saved registers.
             */
            for (int i = status; i >= 0; i--)
                fprintf(f, "POP R%d\n", i);

            count = status;
            break;
        }

        case NODE_WRITE:
        {
            /*
             * Save all currently live registers.
             *
             * IMPORTANT:
             * SP is initialized by codeGen() to an address ABOVE
             * the variable storage, so these PUSHes cannot overwrite
             * variables such as a at 4096.
             */
            for (int i = 0; i <= count; i++)
                fprintf(f, "PUSH R%d\n", i);

            int status = count;

            /*
             * ExpL/XSM Write ABI:
             *
             *   function name = "Write"
             *   arg1          = -2   (integer output)
             *   arg2          = value
             *   arg3          = dummy
             *   return slot   = dummy
             */
            fprintf(f, "MOV R0, \"Write\"\n");
            fprintf(f, "PUSH R0\n");

            fprintf(f, "MOV R0, -2\n");
            fprintf(f, "PUSH R0\n");

            r = expr(t->left, f);
            fprintf(f, "PUSH R%d\n", r);
            freeReg();

            fprintf(f, "PUSH R0\n");
            fprintf(f, "PUSH R0\n");

            fprintf(f, "CALL 0\n");

            /*
             * Remove the 5 values passed to CALL 0.
             */
            fprintf(f, "POP R0\n");
            fprintf(f, "POP R1\n");
            fprintf(f, "POP R1\n");
            fprintf(f, "POP R1\n");
            fprintf(f, "POP R1\n");

            /*
             * Restore saved registers.
             */
            for (int i = status; i >= 0; i--)
                fprintf(f, "POP R%d\n", i);

            count = status;
            break;
        }

        case NODE_IF:
        {
            r = expr(t->left, f);

            l1 = lab();
            l2 = lab();

            fprintf(f, "JZ R%d, L%d\n", r, l1);
            freeReg();

            gen(t->right, f);

            if (t->third != NULL)
            {
                fprintf(f, "JMP L%d\n", l2);
                fprintf(f, "L%d:\n", l1);

                gen(t->third, f);

                fprintf(f, "L%d:\n", l2);
            }
            else
            {
                fprintf(f, "L%d:\n", l1);
            }

            break;
        }

        case NODE_WHILE:
        {
            /*
             * l1 = condition label
             * l2 = loop-exit label
             *
             * continue -> l1
             * break    -> l2
             */
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

            fprintf(f, "JMP L%d\n", l1);
            fprintf(f, "L%d:\n", l2);
            break;
        }

        case NODE_DO_WHILE:
        {
            /*
             * Standard do-while semantics:
             * body executes once before the condition is tested.
             *
             * l1 = body/start label
             * l2 = condition-check label
             * l3 = loop-exit label
             *
             * continue -> l2
             * break    -> l3
             */
            l1 = lab();
            l2 = lab();
            l3 = lab();

            fprintf(f, "L%d:\n", l1);

            if (loopTop >= 1023)
            {
                printf("Too many nested loops\n");
                exit(1);
            }

            loopTop++;
            breakStack[loopTop] = l3;
            continueStack[loopTop] = l2;

            gen(t->right, f);

            loopTop--;

            fprintf(f, "L%d:\n", l2);
            r = expr(t->left, f);
            fprintf(f, "JNZ R%d, L%d\n", r, l1);
            freeReg();
            fprintf(f, "L%d:\n", l3);
            break;
        }

        case NODE_REPEAT_UNTIL:
        {
            /*
             * Standard repeat-until semantics:
             * body executes once, and repeats while condition is false.
             *
             * l1 = body/start label
             * l2 = condition-check label
             * l3 = loop-exit label
             *
             * continue -> l2
             * break    -> l3
             */
            l1 = lab();
            l2 = lab();
            l3 = lab();

            fprintf(f, "L%d:\n", l1);

            if (loopTop >= 1023)
            {
                printf("Too many nested loops\n");
                exit(1);
            }

            loopTop++;
            breakStack[loopTop] = l3;
            continueStack[loopTop] = l2;

            gen(t->right, f);

            loopTop--;

            fprintf(f, "L%d:\n", l2);
            r = expr(t->left, f);
            fprintf(f, "JZ R%d, L%d\n", r, l1);
            freeReg();
            fprintf(f, "L%d:\n", l3);
            break;
        }

        case NODE_BREAK:
        {
            /*
             * Task 4: only generate a jump when inside a loop.
             * Outside a loop, simply ignore break.
             */
            if (loopTop >= 0)
                fprintf(
                    f,
                    "JMP L%d\n",
                    breakStack[loopTop]
                );

            break;
        }

        case NODE_CONTINUE:
        {
            /*
             * Task 4: only generate a jump when inside a loop.
             * Outside a loop, simply ignore continue.
             */
            if (loopTop >= 0)
                fprintf(
                    f,
                    "JMP L%d\n",
                    continueStack[loopTop]
                );

            break;
        }
    }
}

void codeGen(struct tnode *t, FILE *f)
{
    /*
     * Variables occupy:
     *   4096 ... maxVarAddr(t)
     *
     * PUSH increments SP before storing, therefore SP must initially
     * point at the last occupied variable address.
     *
     * Example:
     *   a -> 4096
     *   initial SP = 4096
     *   first PUSH -> 4097
     *
     * This prevents saving registers/syscall arguments from
     * overwriting variable a.
     */
    int stackBase = maxVarAddr(t);

    fprintf(f, "MOV SP, %d\n", stackBase);

    /*
     * Register allocator / loop-label state should be fresh for
     * every code-generation run.
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
        "CALL 0\n"
    );
}