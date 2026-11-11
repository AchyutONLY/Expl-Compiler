int count=-1;
extern FILE *target;

int getReg()
{
  if(count<20)
  {
    count++;
    return count;
  }
  printf("Out of registers/n");
  exit(1);
}

void freeReg()
{
  if(count>=0)
  {
    count--;
  }
}

int codegen(struct tnode *t)
{
  if(t==NULL)
  {
    return -1;
  }
  else if(t->op==NULL)
  {
    int reg1=getReg();
    fprintf(target,"MOV R%d, %d\n",reg1,t->val);
    return reg1;
  }
  else
  {
    int reg1=codegen(t->left);
    int reg2=codegen(t->right);
    switch(*(t->op))
    {
      case '+':
      {
        fprintf(target,"ADD R%d, R%d\n",reg1,reg2);
        freeReg();
        return reg1;
      }
      case '-':
      {
        fprintf(target,"SUB R%d, R%d\n",reg1,reg2);
        freeReg();
        return reg1;
      }
      case '*':
      {
        fprintf(target,"MUL R%d, R%d\n",reg1,reg2);
        freeReg();
        return reg1;
      }
      case '/':
      {
        fprintf(target,"DIV R%d, R%d\n",reg1,reg2);
        freeReg();
        return reg1;
      }
    }
  }
}
