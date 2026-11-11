#include <string.h>

struct tnode *createTree(int type,int val,int nodetype,char *varname,struct tnode *l,struct tnode *r)
{
    struct tnode *temp=(struct tnode *)malloc(sizeof(struct tnode));
    temp->type=type;
    temp->val=val;
    temp->nodetype=nodetype;
    if(varname!=NULL)
    {
        temp->varname=(char *)malloc(strlen(varname)+1);
        strcpy(temp->varname,varname);
    }
    temp->left=l;
    temp->right=r;
    return temp;
}