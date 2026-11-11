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
static struct Classtable *Chead = NULL, *Ctail = NULL;
static int next_class_index = 0;

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
        fprintf(stderr, "Semantic Error: Type '%s' declared multiple times.\n", name);
        exit(1);
    }
    return newType(name, next_user_type++, 0, 0);
}

int FInstall(struct Typetable *owner, char *name, int type) {
    if (!owner) return 0;
    if (owner->fieldtail && owner->fieldtail->fieldIndex >= 7) {
        fprintf(stderr, "too many member fields\n");
        exit(1);
    }
    for (struct Fieldlist *f = owner->fields; f; f = f->next) {
        if (strcmp(f->name, name) == 0) {
            fprintf(stderr, "Semantic Error: Field '%s' declared multiple times in type '%s'.\n", name, owner->name);
            exit(1);
        }
    }
    if (type != 1 && type != 2 && !IsUserType(type) && type != owner->type) {
        fprintf(stderr, "Semantic Error: Invalid field type for '%s' in type '%s'.\n", name, owner->name);
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
        fprintf(stderr, "too many member fields\n");
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

static const char* getTypeName(int type);

void ClassTableCreate(void) {
    Chead = Ctail = NULL;
    next_class_index = 0;
    memory_binding = 4096;
}

struct Classtable *CLookup(char *name) {
    for (struct Classtable *c = Chead; c; c = c->next)
        if (strcmp(c->name, name) == 0) return c;
    return NULL;
}

static struct Paramstruct *CopyParams(const struct Paramstruct *src) {
    struct Paramstruct *head = NULL, *tail = NULL;
    for (const struct Paramstruct *p = src; p; p = p->next) {
        struct Paramstruct *n = calloc(1, sizeof(*n));
        if (!n) { perror("calloc"); exit(1); }
        n->name = strdup(p->name ? p->name : "");
        n->type = p->type;
        if (!head) head = tail = n;
        else { tail->next = n; tail = n; }
    }
    return head;
}

static int SameParamTypes(const struct Paramstruct *a, const struct Paramstruct *b) {
    while (a && b) {
        if (a->type != b->type) return 0;
        a = a->next;
        b = b->next;
    }
    return a == NULL && b == NULL;
}

int IsClassDescendant(struct Classtable *actual, struct Classtable *expected) {
    for (struct Classtable *c = actual; c; c = c->parentptr)
        if (c == expected) return 1;
    return 0;
}

struct Classtable *CInstall(char *name, char *parent_class_name) {
    if (CLookup(name) || TLookup(name)) {
        fprintf(stderr, "Semantic Error: Class '%s' is already defined or conflicts with a type.\n", name);
        exit(1);
    }

    struct Classtable *parent = NULL;
    if (parent_class_name != NULL) {
        parent = CLookup(parent_class_name);
        if (!parent) {
            fprintf(stderr, "Semantic Error: Parent class '%s' must be defined before class '%s'.\n",
                    parent_class_name, name);
            exit(1);
        }
    }

    struct Classtable *c = calloc(1, sizeof(*c));
    if (!c) { perror("calloc"); exit(1); }
    c->name = strdup(name);
    c->parentptr = parent;
    c->class_index = next_class_index++;
    c->vft_binding = memory_binding;
    memory_binding += 8; /* One eight-entry virtual function table per class. */

    /* Copy parent fields so all inherited fields keep identical offsets. */
    if (parent) {
        for (struct Fieldlist *pf = parent->memberfield; pf; pf = pf->next) {
            struct Fieldlist *f = calloc(1, sizeof(*f));
            if (!f) { perror("calloc"); exit(1); }
            f->name = strdup(pf->name);
            f->fieldIndex = pf->fieldIndex;
            f->type = pf->type;
            f->ctype = pf->ctype;
            if (!c->memberfield) c->memberfield = c->fieldtail = f;
            else { c->fieldtail->next = f; c->fieldtail = f; }
            c->fieldcount++;
        }

        /* Copy inherited methods and preserve their virtual-table positions. */
        for (struct Memberfunclist *pm = parent->methods; pm; pm = pm->next) {
            struct Memberfunclist *m = calloc(1, sizeof(*m));
            if (!m) { perror("calloc"); exit(1); }
            m->name = strdup(pm->name);
            m->type = pm->type;
            m->paramlist = CopyParams(pm->paramlist);
            m->funcposition = pm->funcposition;
            m->flabel = pm->flabel;
            m->defined = 1;       /* An inherited method already has a body. */
            m->declared_here = 0;
            m->inherited_from_parent = 1;
            if (!c->methods) c->methods = c->methodtail = m;
            else { c->methodtail->next = m; c->methodtail = m; }
            c->methodcount++;
        }
    }

    if (!Chead) Chead = Ctail = c;
    else { Ctail->next = c; Ctail = c; }
    return c;
}

struct Fieldlist *Class_Flookup(struct Classtable *cptr, char *name) {
    if (!cptr) return NULL;
    for (struct Fieldlist *f = cptr->memberfield; f; f = f->next)
        if (strcmp(f->name, name) == 0) return f;
    return NULL;
}

struct Memberfunclist *Class_Mlookup(struct Classtable *cptr, char *name) {
    if (!cptr) return NULL;
    for (struct Memberfunclist *m = cptr->methods; m; m = m->next)
        if (strcmp(m->name, name) == 0) return m;
    return NULL;
}

struct Memberfunclist *Class_MlookupSignature(struct Classtable *cptr, char *name, struct Paramstruct *params) {
    if (!cptr) return NULL;
    for (struct Memberfunclist *m = cptr->methods; m; m = m->next)
        if (strcmp(m->name, name) == 0 && SameParamTypes(m->paramlist, params)) return m;
    return NULL;
}

void Class_Finstall(struct Classtable *cptr, char *typename, char *name) {
    if (!cptr) { fprintf(stderr, "Internal Error: no current class for field '%s'.\n", name); exit(1); }
    if (cptr->fieldcount >= 8) {
        fprintf(stderr, "Semantic Error: Class '%s' exceeds the maximum of 8 fields including inherited fields.\n", cptr->name);
        exit(1);
    }
    if (Class_Flookup(cptr, name)) {
        fprintf(stderr, "Semantic Error: Field '%s' is declared more than once or hides an inherited field in class '%s'.\n", name, cptr->name);
        exit(1);
    }
    if (Class_Mlookup(cptr, name)) {
        fprintf(stderr, "Semantic Error: '%s' is already a method in class '%s'.\n", name, cptr->name);
        exit(1);
    }

    int type_code = -999;
    struct Classtable *field_class = NULL;
    if (strcmp(typename, "int") == 0) type_code = 1;
    else if (strcmp(typename, "str") == 0) type_code = 2;
    else {
        struct Typetable *t = TLookup(typename);
        if (t && t->defined && t->type >= 6) type_code = t->type;
        else {
            field_class = CLookup(typename);
            if (!field_class && strcmp(typename, cptr->name) == 0) field_class = cptr;
            if (field_class) type_code = CLASS_TYPE;
            else {
                fprintf(stderr, "Semantic Error: Unknown field type '%s' in class '%s'.\n", typename, cptr->name);
                exit(1);
            }
        }
    }

    struct Fieldlist *f = calloc(1, sizeof(*f));
    if (!f) { perror("calloc"); exit(1); }
    f->name = strdup(name);
    f->type = type_code;
    f->ctype = field_class;
    f->fieldIndex = cptr->fieldcount++;
    if (!cptr->memberfield) cptr->memberfield = cptr->fieldtail = f;
    else { cptr->fieldtail->next = f; cptr->fieldtail = f; }
}

struct Memberfunclist *Class_Minstall(struct Classtable *cptr, char *name, int type, struct Paramstruct *paramlist) {
    if (!cptr) { fprintf(stderr, "Internal Error: no current class for method '%s'.\n", name); exit(1); }
    if (Class_Flookup(cptr, name)) {
        fprintf(stderr, "Semantic Error: '%s' is already a field in class '%s'.\n", name, cptr->name);
        exit(1);
    }

    struct Memberfunclist *existing = Class_MlookupSignature(cptr, name, paramlist);
    if (existing) {
        if (existing->declared_here) {
            fprintf(stderr, "Semantic Error: Method '%s' with this parameter signature is declared more than once in class '%s'.\n", name, cptr->name);
            exit(1);
        }
        if (existing->type != type) {
            fprintf(stderr, "Semantic Error: Return type of overridden method '%s' must match the inherited method.\n", name);
            exit(1);
        }
        /* A same-signature declaration overrides the inherited slot. */
        existing->paramlist = CopyParams(paramlist);
        existing->flabel = flabel_counter++;
        existing->defined = 0;
        existing->declared_here = 1;
        return existing;
    }

    /* Exercise 1 intentionally permits only the limited overload case: a child may
       add a different signature for a method name inherited from an ancestor. */
    int has_inherited_name = 0;
    int has_same_name = 0;
    for (struct Memberfunclist *scan = cptr->methods; scan; scan = scan->next) {
        if (strcmp(scan->name, name) == 0) {
            has_same_name = 1;
            if (scan->inherited_from_parent) has_inherited_name = 1;
        }
    }
    if (has_same_name && !has_inherited_name) {
        fprintf(stderr, "Semantic Error: Overloading '%s' is supported only for a method inherited from a parent class.\n", name);
        exit(1);
    }
    if (cptr->methodcount >= 8) {
        fprintf(stderr, "Semantic Error: Class '%s' exceeds the maximum of 8 methods including inherited methods and overloads.\n", cptr->name);
        exit(1);
    }

    /* A same-name/different-signature declaration is a new overload and slot. */
    struct Memberfunclist *m = calloc(1, sizeof(*m));
    if (!m) { perror("calloc"); exit(1); }
    m->name = strdup(name);
    m->type = type;
    m->paramlist = CopyParams(paramlist);
    m->funcposition = cptr->methodcount++;
    m->flabel = flabel_counter++;
    m->defined = 0;
    m->declared_here = 1;
    if (!cptr->methods) cptr->methods = cptr->methodtail = m;
    else { cptr->methodtail->next = m; cptr->methodtail = m; }
    return m;
}

void VerifyMethodSignature(struct Memberfunclist *method, int ret_type, struct Paramstruct *params) {
    if (!method) { fprintf(stderr, "Semantic Error: Method definition has no matching declaration/signature.\n"); exit(1); }
    if (!method->declared_here) {
        fprintf(stderr, "Semantic Error: Inherited method '%s' must be declared in the child before it can be overridden.\n", method->name);
        exit(1);
    }
    if (method->type != ret_type) {
        fprintf(stderr, "Semantic Error: Return type mismatch for method '%s'.\n", method->name); exit(1);
    }
    struct Paramstruct *a = method->paramlist, *b = params;
    while (a && b) {
        if (a->type != b->type || (a->name && b->name && strcmp(a->name, b->name) != 0)) {
            fprintf(stderr, "Semantic Error: Parameter mismatch in method '%s'.\n", method->name); exit(1);
        }
        a = a->next; b = b->next;
    }
    if (a || b) {
        fprintf(stderr, "Semantic Error: Parameter count mismatch in method '%s'.\n", method->name); exit(1);
    }
}

void WriteClassVFTInit(FILE *out) {
    if (!out) return;
    for (struct Classtable *c = Chead; c; c = c->next) {
        for (int i = 0; i < 8; ++i) {
            fprintf(out, "MOV R1, %d\nMOV R2, 0\nMOV [R1], R2\n", c->vft_binding + i);
        }
        for (struct Memberfunclist *m = c->methods; m; m = m->next) {
            fprintf(out, "MOV R1, %d\nMOV R2, F%d\nMOV [R1], R2\n",
                    c->vft_binding + m->funcposition, m->flabel);
        }
    }
}

void PrintClassTable(void) {
    printf("\n============================= CLASS TABLE =============================\n");
    for (struct Classtable *c = Chead; c; c = c->next) {
        printf("Class %s: fields=%d methods=%d parent=%s vft@%d\n", c->name, c->fieldcount,
               c->methodcount, c->parentptr ? c->parentptr->name : "NULL", c->vft_binding);
        for (struct Fieldlist *f = c->memberfield; f; f = f->next) {
            const char *type_name = f->ctype ? f->ctype->name : getTypeName(f->type);
            printf("  field[%d] %-14s : %s%s\n", f->fieldIndex, f->name, type_name,
                   c->parentptr && f->fieldIndex < c->parentptr->fieldcount ? " (inherited)" : "");
        }
        for (struct Memberfunclist *m = c->methods; m; m = m->next) {
            printf("  method[%d] %-13s : %s (F%d)%s%s\n", m->funcposition, m->name,
                   getTypeName(m->type), m->flabel, m->declared_here ? "" : " (inherited)",
                   m->defined ? "" : " [declared, body missing]");
            printf("      signature: (");
            for (struct Paramstruct *p = m->paramlist; p; p = p->next)
                printf("%s%s", getTypeName(p->type), p->next ? ", " : "");
            printf(")\n");
        }
    }
    printf("========================================================================\n");
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
        fprintf(stderr, "Semantic Error: Global '%s' declared multiple times.\n", name);
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

void GInstallClass(char *name, struct Classtable *ctype) {
    if (!ctype) { fprintf(stderr, "Internal Error: invalid class type for global '%s'.\n", name); exit(1); }
    if (GLookUp(name)) {
        fprintf(stderr, "Semantic Error: Global '%s' declared multiple times.\n", name); exit(1);
    }
    struct Gsymbol *g = calloc(1, sizeof(*g));
    if (!g) { perror("calloc"); exit(1); }
    g->name = strdup(name);
    g->type = CLASS_TYPE;
    g->ctype = ctype;
    g->size = 2;
    g->col_size = -2;
    g->binding = memory_binding;
    memory_binding += 2;
    g->flabel = -1;
    if (!Ghead) Ghead = Gtail = g;
    else { Gtail->next = g; Gtail = g; }
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
        fprintf(stderr, "Semantic Error: Local '%s' declared multiple times.\n", name);
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

void LInstallSelf(struct Classtable *ctype, int binding) {
    if (LLookUp("self")) { fprintf(stderr, "Semantic Error: duplicate self binding.\n"); exit(1); }
    struct Lsymbol *s = calloc(1, sizeof(*s));
    if (!s) { perror("calloc"); exit(1); }
    s->name = strdup("self");
    s->type = CLASS_TYPE;
    s->ctype = ctype;
    s->binding = binding;
    if (!Lhead) Lhead = Ltail = s;
    else { Ltail->next = s; Ltail = s; }
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
    printf("%-15s | %-12s | %-4s | %-7s | %-6s | %s\n", "NAME", "TYPE", "SIZE", "BINDING", "FLABEL", "PARAMETERS");
    printf("---------------------------------------------------------------------\n");
    struct Gsymbol* temp = Ghead;
    while (temp != NULL) {
        printf("%-15s | %-12s | %-4d | %-7d | ", temp->name, temp->ctype ? temp->ctype->name : getTypeName(temp->type), temp->size, temp->binding);
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
    printf("%-10s | %-12s | %-7s\n", "NAME", "TYPE", "BINDING");
    printf("----------------------------------\n");
    struct Lsymbol* temp = Lhead;
    while (temp != NULL) {
        printf("%-10s | %-12s | %-7d\n", temp->name, temp->ctype ? temp->ctype->name : getTypeName(temp->type), temp->binding);
        temp = temp->next;
    }
    printf("----------------------------------\n\n");
}