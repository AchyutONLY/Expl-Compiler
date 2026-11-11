#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symboltable.h"

struct Gsymbol* Ghead = NULL;
struct Gsymbol* Gtail = NULL;
struct Lsymbol* Lhead = NULL;
struct Lsymbol* Ltail = NULL;

int memory_binding = 4096;
int flabel_counter = 0;
int local_binding = 1;
int param_binding = -3;

static struct Typetable *Thead = NULL, *Ttail = NULL;
static int next_user_type = 6;

static struct Typetable *newType(char *name, int code, int size, int defined) {
    struct Typetable *t = calloc(1, sizeof(struct Typetable));
    if (!t) { perror("calloc"); exit(1); }
    t->name = strdup(name);
    t->type = code;
    t->size = size;
    t->defined = defined;
    if (Thead == NULL) Thead = Ttail = t;
    else { Ttail->next = t; Ttail = t; }
    return t;
}

void TypeTableCreate(void) {
    Thead = Ttail = NULL;
    next_user_type = 6;
    newType("bool", 0, 0, 1);
    newType("int", 1, 1, 1);
    newType("str", 2, 1, 1);
    newType("int*", 3, 1, 1);
    newType("str*", 4, 1, 1);
    newType("null", 5, 0, 1);
    newType("void", -1, 0, 1);
}

struct Typetable *TLookup(char *name) {
    struct Typetable *t = Thead;
    while (t) {
        if (strcmp(t->name, name) == 0) return t;
        t = t->next;
    }
    return NULL;
}

struct Typetable *TLookupCode(int type) {
    struct Typetable *t = Thead;
    while (t) {
        if (t->type == type) return t;
        t = t->next;
    }
    return NULL;
}

int IsUserType(int type) {
    struct Typetable *t = TLookupCode(type);
    return t != NULL && type >= 6 && t->defined;
}

struct Typetable *TBegin(char *name) {
    if (TLookup(name) != NULL) {
        fprintf(stderr, "Semantic Error: Type '%s' declared multiple times.\\n", name);
        exit(1);
    }
    return newType(name, next_user_type++, 0, 0);
}

int FInstall(struct Typetable *owner, char *name, int type) {
    if (!owner) return 0;
    if (owner->fieldtail && owner->fieldtail->fieldIndex >= 7) {
        fprintf(stderr, "too many member fields\\n");
        exit(1);
    }
    for (struct Fieldlist *f = owner->fields; f; f = f->next) {
        if (strcmp(f->name, name) == 0) {
            fprintf(stderr, "Semantic Error: Field '%s' declared multiple times in type '%s'.\\n", name, owner->name);
            exit(1);
        }
    }
    if (type != 1 && type != 2 && !IsUserType(type) && type != owner->type) {
        fprintf(stderr, "Semantic Error: Invalid field type for '%s' in type '%s'.\\n", name, owner->name);
        exit(1);
    }
    struct Fieldlist *f = calloc(1, sizeof(struct Fieldlist));
    if (!f) { perror("calloc"); exit(1); }
    f->name = strdup(name);
    f->type = type;
    f->fieldIndex = owner->fieldtail ? owner->fieldtail->fieldIndex + 1 : 0;
    if (owner->fields == NULL) owner->fields = owner->fieldtail = f;
    else { owner->fieldtail->next = f; owner->fieldtail = f; }
    owner->size++;
    return 1;
}

int TFinish(struct Typetable *owner) {
    if (!owner) return 0;
    if (owner->size > 8) {
        fprintf(stderr, "too many member fields\\n");
        exit(1);
    }
    owner->defined = 1;
    return 1;
}

struct Fieldlist *FLookup(struct Typetable *type, char *name) {
    if (!type) return NULL;
    struct Fieldlist *f = type->fields;
    while (f) {
        if (strcmp(f->name, name) == 0) return f;
        f = f->next;
    }
    return NULL;
}

int GetSize(struct Typetable *type) {
    return type ? type->size : 0;
}

int IsCompatibleType(int expected, int actual) {
    if (expected == actual) return 1;
    if (IsUserType(expected) && actual == 5) return 1; /* NULL */
    return 0;
}

struct Gsymbol* GLookUp(char *name) {
    struct Gsymbol* temp = Ghead;
    while (temp != NULL) {
        if (strcmp(temp->name, name) == 0) return temp;
        temp = temp->next;
    }
    return NULL;
}

void GInstall(char* name, int type, int size, int col_size, struct Paramstruct* paramlist) {
    if (GLookUp(name) != NULL) {
        fprintf(stderr, "Semantic Error: Global '%s' declared multiple times.\\n", name);
        exit(1);
    }
    struct Gsymbol* new_node = calloc(1, sizeof(struct Gsymbol));
    if (!new_node) { perror("calloc"); exit(1); }
    new_node->name = strdup(name);
    new_node->type = type;
    new_node->size = size;
    new_node->col_size = col_size;
    new_node->paramlist = paramlist;
    if (size >= 0) {
        new_node->binding = memory_binding;
        memory_binding += size;
        new_node->flabel = -1;
    } else {
        new_node->binding = -1;
        new_node->flabel = flabel_counter++;
    }
    new_node->next = NULL;
    if (Ghead == NULL) Ghead = Gtail = new_node;
    else { Gtail->next = new_node; Gtail = new_node; }
}

struct Lsymbol* LLookUp(char *name) {
    struct Lsymbol* temp = Lhead;
    while (temp != NULL) {
        if (strcmp(temp->name, name) == 0) return temp;
        temp = temp->next;
    }
    return NULL;
}

void LInstall(char* name, int type, int is_param) {
    if (LLookUp(name) != NULL) {
        fprintf(stderr, "Semantic Error: Local '%s' declared multiple times.\\n", name);
        exit(1);
    }
    struct Lsymbol* new_node = calloc(1, sizeof(struct Lsymbol));
    if (!new_node) { perror("calloc"); exit(1); }
    new_node->name = strdup(name);
    new_node->type = type;
    if (is_param) new_node->binding = param_binding--;
    else new_node->binding = local_binding++;
    new_node->next = NULL;
    if (Lhead == NULL) Lhead = Ltail = new_node;
    else { Ltail->next = new_node; Ltail = new_node; }
}

void ClearLST(void) {
    struct Lsymbol* temp = Lhead;
    while (temp != NULL) {
        struct Lsymbol* next = temp->next;
        free(temp->name);
        free(temp);
        temp = next;
    }
    Lhead = Ltail = NULL;
    local_binding = 1;
    param_binding = -3;
}

void VerifyFunctionSignature(char* name, int ret_type, struct Paramstruct* def_params) {
    struct Gsymbol* g = GLookUp(name);
    if (g == NULL) { fprintf(stderr, "Error: Function %s not declared.\n", name); exit(1); }
    if (g->type != ret_type) { fprintf(stderr, "Error: Return type mismatch for %s.\n", name); exit(1); }
    struct Paramstruct* dec = g->paramlist;
    struct Paramstruct* def = def_params;
    while (dec != NULL && def != NULL) {
        if (strcmp(dec->name, def->name) != 0 || dec->type != def->type) {
            fprintf(stderr, "Error: Parameter mismatch for function %s.\n", name); exit(1);
        }
        dec = dec->next;
        def = def->next;
    }
    if (dec != NULL || def != NULL) {
        fprintf(stderr, "Error: Parameter count mismatch for function %s.\n", name); exit(1);
    }
}

static const char* getTypeName(int type) {
    struct Typetable *t = TLookupCode(type);
    if (t) return t->name;
    return "unknown";
}

void PrintGST(void) {
    printf("\n======================== GLOBAL SYMBOL TABLE ========================\n");
    printf("%-15s | %-12s | %-4s | %-7s | %-6s | %s\\n", "NAME", "TYPE", "SIZE", "BINDING", "FLABEL", "PARAMETERS");
    printf("---------------------------------------------------------------------\n");
    struct Gsymbol* temp = Ghead;
    while (temp != NULL) {
        printf("%-15s | %-12s | %-4d | %-7d | ", temp->name, getTypeName(temp->type), temp->size, temp->binding);
        if (temp->flabel != -1) printf("F%-5d | ", temp->flabel);
        else printf("%-6s | ", "-");
        struct Paramstruct* p = temp->paramlist;
        if (p == NULL) printf("-");
        else {
            while (p != NULL) {
                printf("%s %s", getTypeName(p->type), p->name);
                if (p->next != NULL) printf(", ");
                p = p->next;
            }
        }
        printf("\n");
        temp = temp->next;
    }
    printf("=====================================================================\n\n");
}

void PrintLST(const char* scopeName) {
    printf("\n--- Local Symbol Table (%s) ---\n", scopeName);
    printf("%-10s | %-12s | %-7s\\n", "NAME", "TYPE", "BINDING");
    printf("----------------------------------\n");
    struct Lsymbol* temp = Lhead;
    while (temp != NULL) {
        printf("%-10s | %-12s | %-7d\n", temp->name, getTypeName(temp->type), temp->binding);
        temp = temp->next;
    }
    printf("----------------------------------\n\n");
}