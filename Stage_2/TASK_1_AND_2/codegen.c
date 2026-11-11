int count=-1;
int i,j;
extern FILE *target;

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

int freeReg()
{
    if(count>=0)
    {
        count--;
    }
}

int codegen(struct tnode *t)
{
    int r1,r2;
    int number;
    int status=0;
    if(t==NULL)
    {
        return -1;
    }
    else if(t->nodetype==NODE_CONNECTOR)
    {
        codegen(t->left);
        codegen(t->right);
    }
    switch(t->nodetype)
    {
        case NODE_NUM:
        {
            r1=getReg();
            fprintf(target,"MOV R%d, %d\n",r1,t->val);
            return r1;
        }
        case NODE_ID:
        {
            r1=getReg();
            number=4096+t->varname[0]-'a';
            fprintf(target,"MOV R%d, [%d]\n",r1,number);
            return r1;
        }
        case NODE_PLUS:
        {
            r1=codegen(t->left);
            r2=codegen(t->right);
            fprintf(target,"ADD R%d, R%d\n",r1,r2);
            freeReg();
            return r1;
        }
        case NODE_MINUS:
        {
            r1=codegen(t->left);
            r2=codegen(t->right);
            fprintf(target,"SUB R%d, R%d\n",r1,r2);
            freeReg();
            return r1;
        }
        case NODE_MUL:
        {
            r1=codegen(t->left);
            r2=codegen(t->right);
            fprintf(target,"MUL R%d, R%d\n",r1,r2);
            freeReg();
            return r1;
        }
        case NODE_DIV:
        {
            r1=codegen(t->left);
            r2=codegen(t->right);
            fprintf(target,"DIV R%d, R%d\n",r1,r2);
            freeReg();
            return r1;
        }
        case NODE_ASSGN:
        {
            number=4096+t->left->varname[0]-'a';
            r2=codegen(t->right);
            fprintf(target,"MOV [%d], R%d\n",number,r2);
            freeReg();
            return 0;
        }
        case NODE_WRITE:
        {
            for(i=0;i<=count;i++)
            {
                fprintf(target,"PUSH R%d\n",i);
            }
            status=count;

            fprintf(target,"MOV R0,\"Write\"\n");
            fprintf(target,"PUSH R0\n");
            fprintf(target,"MOV R0,-1\n");
            fprintf(target,"PUSH R0\n");
            r1=codegen(t->left);
            fprintf(target,"PUSH R%d\n",r1);
            freeReg();
            fprintf(target, "PUSH R0\n");
	        fprintf(target, "PUSH R0\n");
            fprintf(target,"CALL 0\n");
            fprintf(target, "POP R0\n");
            fprintf(target, "POP R1\n");
            fprintf(target, "POP R1\n");
            fprintf(target, "POP R1\n");
            fprintf(target, "POP R1\n");

            for(i=status;i>=0;i--)
            {
                fprintf(target,"POP R%d\n",i);
            }
            count=status;
            break;
        }
        case NODE_READ:
        {
            number=4096+t->left->varname[0]-'a';
            for(i=0;i<=count;i++)
            {
                fprintf(target,"PUSH R%d\n",i);
            }
            status=count;

            fprintf(target,"MOV R0,\"Read\"\n");
            fprintf(target,"PUSH R0\n");
            fprintf(target,"MOV R0,-1\n");
            fprintf(target,"PUSH R0\n");
            fprintf(target,"MOV R0,%d\n",number);
            fprintf(target, "PUSH R0\n");
	        fprintf(target, "PUSH R0\n");
            fprintf(target, "PUSH R0\n");
            fprintf(target,"CALL 0\n");
            fprintf(target, "POP R0\n");
            fprintf(target, "POP R1\n");
            fprintf(target, "POP R1\n");
            fprintf(target, "POP R1\n");
            fprintf(target, "POP R1\n");
        }

        for(i=status;i>=0;i--)
        {
            fprintf(target,"POP R%d\n",i);
        }
        count=status;
        break;
    }
}