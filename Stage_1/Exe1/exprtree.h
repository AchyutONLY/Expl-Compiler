typedef struct tnode
{
  int val;
  char *op;
  struct tnode *left;
  struct tnode *right;
}tnode;
struct tnode * makeLeafNode(int);
struct tnode * makeOperatorNode(char,struct tnode *,struct tnode *);
