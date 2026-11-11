#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

struct Fieldlist {
    char *name;
    int fieldIndex;
    int type;
    struct Fieldlist *next;
};

struct Typetable {
    char *name;
    int type;
    int size;
    int defined;
    struct Fieldlist *fields;
    struct Fieldlist *fieldtail;
    struct Typetable *next;
};

struct Paramstruct {
    char* name;
    int type;
    struct Paramstruct* next;
};

struct Gsymbol {
    char* name;
    int type;
    int size;
    int col_size;
    int binding;
    struct Paramstruct *paramlist;
    int flabel;
    struct Gsymbol* next;
};

/* Defined in symboltable.c and used by parser.y during code generation. */
extern struct Gsymbol* Ghead;

struct Lsymbol {
    char* name;
    int type;
    int binding;
    struct Lsymbol* next;
};

struct Gsymbol* GLookUp(char *name);
void GInstall(char* name, int type, int size, int col_size, struct Paramstruct* paramlist);

struct Lsymbol* LLookUp(char *name);
void LInstall(char* name, int type, int is_param);
void ClearLST(void);

void VerifyFunctionSignature(char* name, int ret_type, struct Paramstruct* def_params);
void PrintGST(void);
void PrintLST(const char* scopeName);

void TypeTableCreate(void);
struct Typetable *TLookup(char *name);
struct Typetable *TLookupCode(int type);
struct Typetable *TBegin(char *name);
int FInstall(struct Typetable *owner, char *name, int type);
int TFinish(struct Typetable *owner);
struct Fieldlist *FLookup(struct Typetable *type, char *name);
int GetSize(struct Typetable *type);
int IsUserType(int type);
int IsCompatibleType(int expected, int actual);

#endif