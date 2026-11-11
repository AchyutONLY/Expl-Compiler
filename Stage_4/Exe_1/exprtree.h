#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_BOOL 0
#define TYPE_INT 1
#define TYPE_STR 2

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
#define NODE_STR 22
#define NODE_ARRAY 23

struct Gsymbol
{
    char *name;
    int type;
    int size;
    int isArray;
    int dims;
    int dim1;
    int dim2;
    int binding;
    struct Gsymbol *next;
};

struct tnode
{
    int val;
    int type;
    char *varname;
    char *strval;
    int nodetype;
    struct Gsymbol *Gentry;
    struct tnode *left, *right, *third;
};

extern struct Gsymbol *Ghead;
extern int nextBinding;

struct Gsymbol *Lookup(char *);
void Install(char *, int, int, int, int, int, int);
void PrintGsymbolTable(void);

struct tnode *createTree(int, int, char *, char *, int, struct Gsymbol *,
                         struct tnode *, struct tnode *, struct tnode *);

void codeGen(struct tnode *, FILE *);
void emit_exit(FILE *);