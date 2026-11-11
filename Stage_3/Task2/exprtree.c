int labels = 0;
int count=0;

int getReg()
{
    if(count<20)
    {
        count++;
        return count;
    }
    printf("Out of registers]\n");
    exit(1);
}

void freeReg()
{
    if(count>=0)
    {
        count--;
    }
}


int lab() 
{
    return labels++; 
}

int addr(char *s) 
{ 
    return 4096 + s[0] - 'a'; 
}

struct tnode *createTree(int val,int type,char *varname,int nodetype,struct tnode *left,struct tnode *right,struct tnode *third)
{
    struct tnode *newnode=(struct tnode *)malloc(sizeof(struct tnode));

    if(newnode==NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    newnode->val=val;
    newnode->type=type;
    if(varname!=NULL)
    {
        newnode->varname=(char *)malloc(strlen(varname)+1);
        strcpy(newnode->varname,varname);
    }
    else
    {
        newnode->varname=NULL;
    }
    newnode->nodetype=nodetype;
    
    if(nodetype==NODE_PLUS || nodetype==NODE_MINUS || nodetype==NODE_MUL || nodetype==NODE_DIV || nodetype==NODE_MOD)
    {
        if(left->type!=TYPE_INT || right->type!=TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        newnode->type=TYPE_INT;
    }
    else if(nodetype==NODE_LT || nodetype==NODE_GT || nodetype==NODE_LE || nodetype==NODE_GE || nodetype==NODE_EQ || nodetype==NODE_NE)
    {
        if(left->type!=TYPE_INT || right->type!=TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        newnode->type=TYPE_BOOL;
    }
    else if(nodetype==NODE_ASSGN)
    {
        if(left->type!=TYPE_INT || right->type!=TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        newnode->type=TYPE_INT;
    }
    else if(nodetype==NODE_READ || nodetype==NODE_WRITE)
    {
        if(left->type!=TYPE_INT)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        newnode->type=-1;
    }
    else if(nodetype==NODE_IF || nodetype==NODE_WHILE)
    {
        if(left->type!=TYPE_BOOL)
        {
            printf("Type Mismatch\n");
            exit(1);
        }
        newnode->type=-1;
    }
    else if(nodetype==NODE_NUM || nodetype==NODE_ID)
    {
        newnode->type=TYPE_INT;
    }

    newnode->left=left;
    newnode->right=right;
    newnode->third=third;
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
        fprintf(f, "MOV R%d, [%d]\n", a, addr(t->varname));
        return a;
    }
    a = expr(t->left, f);
    b = expr(t->right, f);
    const char *op = "";
    switch (t->nodetype)
    {
        case NODE_PLUS:
        {
            op = "ADD";
            break;
        }
        case NODE_MINUS:
        {
            op = "SUB";
            break;
        }
        case NODE_MUL:
        {
            op = "MUL";
            break;
        }
        case NODE_DIV:
        {
            op = "DIV";
            break;
        }
        case NODE_MOD:
        {
            op = "MOD";
            break;
        }
        case NODE_LT:
        {
            op = "LT";
            break;
        }
        case NODE_GT:
        {
            op = "GT";
            break;
        }
        case NODE_LE:
        {
            op = "LE";
            break;
        }
        case NODE_GE:
        {
            op = "GE";
            break;
        }
        case NODE_EQ:
        {
            op = "EQ";
            break;
        }
        case NODE_NE:
        {
            op = "NE";
            break;
        }
    }
    fprintf(f, "%s R%d, R%d\n", op, a, b);
    freeReg();
    return a;
}

void gen(struct tnode *t, FILE *f)
{
    if (t==NULL)
    {
        return;
    }
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
            fprintf(f, "MOV [%d], R%d\n", addr(t->left->varname), r);
            freeReg();
            break;
        }
        case NODE_READ:
        {
            q = getReg();
            fprintf(f, "MOV R%d, \"Read\"\nPUSH R%d\nMOV R%d, -1\nPUSH R%d\nMOV R%d, %d\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", q, q, q, q, q, addr(t->left->varname), q, q, q, q, q, q, q);
            freeReg();
            break;
        }
        case NODE_WRITE:
        {
            r = expr(t->left, f);
            q = getReg();
            fprintf(f, "MOV R%d, \"Write\"\nPUSH R%d\nMOV R%d, -2\nPUSH R%d\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", q, q, q, q, r, q, q, q, q, q, q, q);
            freeReg();
            freeReg();
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
            if (t->third)
            {
                fprintf(f, "JMP L%d\nL%d:\n", l2, l1);
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
            l1 = lab();
            l2 = lab();
            fprintf(f, "L%d:\n", l1);
            r = expr(t->left, f);
            fprintf(f, "JZ R%d, L%d\n", r, l2);
            freeReg();
            gen(t->right, f);
            fprintf(f, "JMP L%d\nL%d:\n", l1, l2);
            break;
        }
    }
}
void codeGen(struct tnode *t, FILE *f) 
{
    gen(t, f); 
}
void emit_exit(FILE *f) 
{ 
    fprintf(f, "MOV R1, \"Exit\"\nPUSH R1\nMOV R1, -1\nPUSH R1\nPUSH R1\nPUSH R1\nPUSH R1\nCALL 0\n"); 
}