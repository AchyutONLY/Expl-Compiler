#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BOOL 0
#define TYPE_INT 1
#define NODE_NUM 1
#define NODE_ID 2
#define NODE_PLUS 3
#define NODE_MINUS 4
#define NODE_MUL 5
#define NODE_DIV 6
#define NODE_MOD 7
#define NODE_LT 8
#define NODE_GT 9
#define NODE_LE 10
#define NODE_GE 11
#define NODE_EQ 12
#define NODE_NE 13
#define NODE_ASSGN 14
#define NODE_READ 15
#define NODE_WRITE 16
#define NODE_IF 17
#define NODE_WHILE 18
#define NODE_CONNECTOR 19
#define NODE_BREAK 20
#define NODE_CONTINUE 21

struct tnode
{
    int val,type;
    char *varname;
    int nodetype;
    struct tnode *left,*right,*third;
};
struct tnode *createTree(int,int,char*,int,struct tnode*,struct tnode*,struct tnode*);
void codeGen(struct tnode*,FILE*);
void emit_exit(FILE*);