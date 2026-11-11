#include <stdlib.h>
#include "exprtree.h"

struct tnode * makeLeafNode(int n)
{
  struct tnode *temp=(struct tnode *)malloc(sizeof(struct tnode));
  temp->op=NULL;
  temp->val=n;
  temp->left=NULL;
  temp->right=NULL;
  return temp;
}

struct tnode * makeOperatorNode(char c,struct tnode *l,struct tnode *r)
{
  struct tnode *temp=(struct tnode *)malloc(sizeof(struct tnode));
  temp->op=malloc(sizeof(char));
  *(temp->op)=c;
  temp->left=l;
  temp->right=r;
  return temp;
}
