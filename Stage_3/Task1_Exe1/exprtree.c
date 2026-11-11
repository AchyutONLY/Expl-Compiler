#include <string.h>
int variables[26];

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

int evaluate(struct tnode *t)
{
    if(t==NULL)
    {
        return 0;
    }
    if(t->nodetype==NODE_NUM)
    {
        return t->val;
    }
    else if(t->nodetype==NODE_ID)
    {
        int index=t->varname[0]-'a';
        return variables[index];
    }
    else if(t->nodetype==NODE_CONNECTOR)
    {
        evaluate(t->left);
        evaluate(t->right);
        return 0;
    }
    else if(t->nodetype==NODE_READ)
    {
        int index=t->left->varname[0]-'a';
        scanf("%d",&variables[index]);
        return 0;
    }
    else if(t->nodetype==NODE_WRITE)
    {
        int temp=evaluate(t->left);
        printf("%d\n",temp);
        return 0;
    }
    else if(t->nodetype==NODE_ASSGN)
    {
        int temp=evaluate(t->right);
        int index=t->left->varname[0]-'a';
        variables[index]=temp;
        return 0;
    }
    else if(t->nodetype==NODE_PLUS)
    {
        return evaluate(t->left)+evaluate(t->right);
    }
    else if(t->nodetype==NODE_MINUS)
    {
        return evaluate(t->left)-evaluate(t->right);
    }
    else if(t->nodetype==NODE_MUL)
    {
        return evaluate(t->left)*evaluate(t->right);
    }
    else if(t->nodetype==NODE_DIV)
    {
        return evaluate(t->left)/evaluate(t->right);
    }
    else if(t->nodetype==NODE_MOD)
    {
        return evaluate(t->left)%evaluate(t->right);
    }
    else if(t->nodetype==NODE_LT)
    {
        return evaluate(t->left)<evaluate(t->right);
    }
    else if(t->nodetype==NODE_GT)
    {
        return evaluate(t->left)>evaluate(t->right);
    }
    else if(t->nodetype==NODE_LE)
    {
        return evaluate(t->left)<=evaluate(t->right);
    }
    else if(t->nodetype==NODE_GE)
    {
        return evaluate(t->left)>=evaluate(t->right);
    }
    else if(t->nodetype==NODE_EQ)
    {
        return evaluate(t->left)==evaluate(t->right);
    }
    else if(t->nodetype==NODE_NE)
    {
        return evaluate(t->left)!=evaluate(t->right);
    }
    else if(t->nodetype==NODE_IF)
    {
        if(evaluate(t->left))
        {
            evaluate(t->right);
        }
        else
        {
            if(t->third!=NULL)
            {
                evaluate(t->third);
            }
        }
        return 0;
    }
    else if(t->nodetype==NODE_WHILE)
    {
        while(evaluate(t->left))
        {
            evaluate(t->right);
        }
        return 0;
    }
    return 0;
}