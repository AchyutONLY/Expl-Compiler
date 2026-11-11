#define NODE_CONNECTOR 0
#define NODE_PLUS 1
#define NODE_MINUS 2
#define NODE_MUL 3
#define NODE_DIV 4
#define NODE_ID 5
#define NODE_NUM 6
#define NODE_ASSGN 7
#define NODE_READ 8
#define NODE_WRITE 9

typedef struct tnode
{
    int val;
    int type;
    char *varname;
    int nodetype;
    struct tnode *left, *right;
}tnode;

struct tnode *createTree(int tyep,int val,int nodetype,char *varname,struct tnode *l,struct tnode *r);
int codegen(struct tnode *t);
void initialize();