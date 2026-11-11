#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_INT 1
#define TYPE_STR 2

typedef struct Gsymbol {
    char *name;
    int type;
    int size;
    int binding;
    struct Gsymbol *next;
} Gsymbol;

extern Gsymbol *Ghead;
extern int nextBinding;

Gsymbol *Lookup(char *name);
void Install(char *name, int type, int size);
void PrintGsymbolTable(void);