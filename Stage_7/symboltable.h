#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

struct Classtable;

struct Fieldlist {
    char *name;
    int fieldIndex;
    int type;                    /* Typetable code, or CLASS_TYPE for class fields */
    struct Classtable *ctype;    /* Non-NULL when this field is of class type */
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

/* Stage 7: classes (inheritance is intentionally not implemented). */
#define CLASS_TYPE (-2)

struct Memberfunclist {
    char *name;
    int type;
    struct Paramstruct *paramlist;
    int funcposition;
    int flabel;
    int defined;
    struct Memberfunclist *next;
};

struct Classtable {
    char *name;
    struct Fieldlist *memberfield;
    struct Fieldlist *fieldtail;
    struct Memberfunclist *methods;
    struct Memberfunclist *methodtail;
    struct Classtable *parentptr; /* Always NULL in Stage 7 */
    int class_index;
    int fieldcount;
    int methodcount;
    struct Classtable *next;
};


struct Gsymbol {
    char* name;
    int type;
    struct Classtable *ctype;
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
    struct Classtable *ctype;
    int binding;
    struct Lsymbol* next;
};

struct Gsymbol* GLookUp(char *name);
void GInstall(char* name, int type, int size, int col_size, struct Paramstruct* paramlist);
void GInstallClass(char *name, struct Classtable *ctype);

struct Lsymbol* LLookUp(char *name);
void LInstall(char* name, int type, int is_param);
void LInstallSelf(struct Classtable *ctype, int binding);
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

void ClassTableCreate(void);
struct Classtable *CInstall(char *name, char *parent_class_name);
struct Classtable *CLookup(char *name);
struct Fieldlist *Class_Flookup(struct Classtable *cptr, char *name);
struct Memberfunclist *Class_Mlookup(struct Classtable *cptr, char *name);
void Class_Finstall(struct Classtable *cptr, char *typename, char *name);
struct Memberfunclist *Class_Minstall(struct Classtable *cptr, char *name, int type, struct Paramstruct *paramlist);
void VerifyMethodSignature(struct Memberfunclist *method, int ret_type, struct Paramstruct *params);
void PrintClassTable(void);


#endif