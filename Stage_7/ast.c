#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

int reg_counter = -1;
int label_counter = -1;
int while_start_stack[100];
int while_end_stack[100];
int while_top = -1;

extern int local_binding; 

struct tnode* createTree(int val, int type, char* varname, int nodetype, char* op, struct tnode *l, struct tnode *m, struct tnode *r) {
    struct tnode *temp = (struct tnode*)malloc(sizeof(struct tnode));
    temp->val = val;
    temp->type = type;
    temp->varname = varname != NULL ? strdup(varname) : NULL;
    temp->op = op != NULL ? strdup(op) : NULL;
    temp->nodetype = nodetype;
    temp->Gentry = NULL;
    temp->Lentry = NULL;
    temp->Ctype = NULL;
    temp->Mentry = NULL;
    temp->Fentry = NULL;
    temp->arglist = NULL;
    temp->argnext = NULL;
    temp->left = l;
    temp->mid = m;       
    temp->right = r;
    return temp;
}

int getReg() {
    if (reg_counter < 19) return ++reg_counter;
    printf("Error: Out of registers!\n");
    exit(1);
}

void freeReg() {
    if (reg_counter >= 0) reg_counter--;
}

int getLabel() {
    return ++label_counter;
}

int getAddrCodeGen(struct tnode *t, FILE *target_file) {
    if (t->nodetype == NODE_DEREF)
        return codeGen(t->left, target_file);

    if (t->nodetype == NODE_SELF) {
        int r = getReg();
        int offset = t->Lentry ? t->Lentry->binding : -3;
        fprintf(target_file, "MOV R%d, BP\n", r);
        if (offset > 0) {
            int o = getReg();
            fprintf(target_file, "MOV R%d, %d\nADD R%d, R%d\n", o, offset, r, o);
            freeReg();
        } else {
            int o = getReg();
            fprintf(target_file, "MOV R%d, %d\nSUB R%d, R%d\n", o, -offset, r, o);
            freeReg();
        }
        return r;
    }

    if (t->nodetype == NODE_CLASS_FIELD) {
        /* The base is a class instance pointer (self, a global object, or another class field). */
        int base = codeGen(t->left, target_file);
        if (t->val != 0) {
            int offset = getReg();
            fprintf(target_file, "MOV R%d, %d\nADD R%d, R%d\n", offset, t->val, base, offset);
            freeReg();
        }
        return base;
    }

    if (t->nodetype == NODE_FIELD) {
        int base = getAddrCodeGen(t->left, target_file);
        /* A user-defined variable stores a pointer to its heap object. */
        fprintf(target_file, "MOV R%d, [R%d]\n", base, base);
        if (t->val != 0) {
            int offset = getReg();
            fprintf(target_file, "MOV R%d, %d\nADD R%d, R%d\n", offset, t->val, base, offset);
            freeReg();
        }
        return base;
    }

    int r = getReg();
    if (t->nodetype == NODE_ID) {
        if (t->Lentry != NULL) {
            int offset = t->Lentry->binding;
            fprintf(target_file, "MOV R%d, BP\n", r);
            if (offset > 0) {
                int o = getReg();
                fprintf(target_file, "MOV R%d, %d\nADD R%d, R%d\n", o, offset, r, o);
                freeReg();
            } else {
                int o = getReg();
                fprintf(target_file, "MOV R%d, %d\nSUB R%d, R%d\n", o, -offset, r, o);
                freeReg();
            }
        } else if (t->Gentry != NULL) {
            fprintf(target_file, "MOV R%d, %d\n", r, t->Gentry->binding);
        }
        return r;
    } else if (t->nodetype == NODE_ARRAY) {
        int offset = codeGen(t->left, target_file);
        fprintf(target_file, "MOV R%d, %d\n", r, t->Gentry->binding);
        fprintf(target_file, "ADD R%d, R%d\n", r, offset);
        freeReg();
        return r;
    } else if (t->nodetype == NODE_ARRAY2D) {
        int row = codeGen(t->left, target_file);
        int col = codeGen(t->mid, target_file);
        int temp_reg = getReg();
        fprintf(target_file, "MOV R%d, %d\n", temp_reg, t->Gentry->col_size);
        fprintf(target_file, "MUL R%d, R%d\n", row, temp_reg);
        freeReg();
        fprintf(target_file, "ADD R%d, R%d\n", row, col);
        freeReg();
        fprintf(target_file, "MOV R%d, %d\n", r, t->Gentry->binding);
        fprintf(target_file, "ADD R%d, R%d\n", r, row);
        freeReg();
        return r;
    }
    fprintf(stderr, "Error: Attempted to get address of non-addressable node type.\n");
    exit(1);
}

/*
 * Invoke one of the ExpL runtime-library functions.  The ABI passes:
 * function name, arg1, arg2, arg3, return slot, then CALL 0.
 * All currently-live registers are saved because the library may clobber them.
 * arg_kind: 0 = no arg, 1 = register arg, 2 = immediate integer arg.
 */
static int callLibrary(FILE *out, const char *function, int arg_kind, int arg_reg, int arg_value) {
    int old_top = reg_counter;
    for (int i = 0; i <= old_top; i++)
        fprintf(out, "PUSH R%d\n", i);

    int result = getReg();
    int scratch = (result == 19) ? 18 : 19;
    fprintf(out, "MOV R%d, \"%s\"\nPUSH R%d\n", result, function, result);

    if (arg_kind == 1) {
        fprintf(out, "PUSH R%d\n", arg_reg);
    } else if (arg_kind == 2) {
        fprintf(out, "MOV R%d, %d\nPUSH R%d\n", scratch, arg_value, scratch);
    } else {
        fprintf(out, "MOV R%d, 0\nPUSH R%d\n", scratch, scratch);
    }
    fprintf(out, "MOV R%d, 0\nPUSH R%d\nPUSH R%d\nPUSH R%d\nCALL 0\n",
            scratch, scratch, scratch, scratch);
    fprintf(out, "POP R%d\n", result);
    fprintf(out, "POP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n",
            scratch, scratch, scratch, scratch);

    for (int i = old_top; i >= 0; i--)
        fprintf(out, "POP R%d\n", i);
    reg_counter = old_top + 1;
    return result;
}

int codeGen(struct tnode *t, FILE *target_file) {
    if (t == NULL) return -1;
    int p, q, temp, label_1, label_2;

    switch (t->nodetype) {
        case NODE_NUM:
            p = getReg();
            fprintf(target_file, "MOV R%d, %d\n", p, t->val);
            return p;

        case NODE_STR:
            p = getReg();
            fprintf(target_file, "MOV R%d, %s\n", p, t->varname); 
            return p;

        case NODE_NULL:
            p = getReg();
            fprintf(target_file, "MOV R%d, 0\n", p);
            return p;

        case NODE_ID:
        case NODE_ARRAY:
        case NODE_ARRAY2D:
        case NODE_FIELD:
        case NODE_CLASS_FIELD:
            p = getAddrCodeGen(t, target_file);
            fprintf(target_file, "MOV R%d, [R%d]\n", p, p);
            return p;

        case NODE_SELF:
            p = getAddrCodeGen(t, target_file);
            fprintf(target_file, "MOV R%d, [R%d]\n", p, p);
            return p;

        case NODE_AMPER:
            return getAddrCodeGen(t->left, target_file);

        case NODE_DEREF:
            p = codeGen(t->left, target_file); 
            fprintf(target_file, "MOV R%d, [R%d]\n", p, p);
            return p;

        case NODE_ALLOC: {
            struct Typetable *type_entry = TLookupCode(t->type);
            int size = GetSize(type_entry);
            if (size < 1) size = 1;
            return callLibrary(target_file, "Alloc", 2, -1, size);
        }

        case NODE_NEW:
            /* Every object gets an eight-word heap block, as specified by Stage 7. */
            return callLibrary(target_file, "Alloc", 2, -1, 8);

        case NODE_FREE: {
            p = codeGen(t->left, target_file);
            q = callLibrary(target_file, "Free", 1, p, 0);
            /* The argument register is dead after Free; reuse it for the result. */
            fprintf(target_file, "MOV R%d, R%d\n", p, q);
            freeReg();
            return p;
        }

        case NODE_FREE_STMT: {
            int old_top = reg_counter;
            p = codeGen(t->left, target_file);
            int before_call = reg_counter;
            q = callLibrary(target_file, "Free", 1, p, 0);
            (void)q;
            reg_counter = before_call;
            if (reg_counter > old_top) reg_counter = old_top + 1;
            freeReg();
            return -1;
        }

        case NODE_INIT: {
            /* Statement form: initialize the heap and discard the return value. */
            int old_top = reg_counter;
            q = callLibrary(target_file, "Heapset", 0, -1, 0);
            (void)q;
            reg_counter = old_top;
            return -1;
        }

        case NODE_INIT_EXPR:
            /* Expression form used by the official Stage 6 test: x = initialize();
               Heapset returns integer 0, which is assigned to x. */
            return callLibrary(target_file, "Heapset", 0, -1, 0);

        case NODE_ASSIGN:
            p = getAddrCodeGen(t->left, target_file); 
            q = codeGen(t->right, target_file);       
            fprintf(target_file, "MOV [R%d], R%d\n", p, q);
            freeReg(); freeReg(); 
            return -1;

        case NODE_READ:
            p = getAddrCodeGen(t->left, target_file); 
            q = getReg();
            fprintf(target_file, "MOV R%d, \"Read\"\n", q);
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "MOV R%d, -1\n", q);
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "PUSH R%d\n", p); 
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "CALL 0\n");
            fprintf(target_file, "POP R%d\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", q, q, q, q, q);
            freeReg(); freeReg(); 
            return -1;

        case NODE_WRITE:
            p = codeGen(t->left, target_file);
            q = getReg();
            fprintf(target_file, "MOV R%d, \"Write\"\n", q);
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "MOV R%d, -2\n", q);
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "PUSH R%d\n", p); 
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "PUSH R%d\n", q);
            fprintf(target_file, "CALL 0\n");
            fprintf(target_file, "POP R%d\nPOP R%d\nPOP R%d\nPOP R%d\nPOP R%d\n", q, q, q, q, q);
            freeReg(); freeReg();
            return -1;

        case NODE_CONN:
            codeGen(t->left, target_file);
            codeGen(t->right, target_file);
            return -1;

        case NODE_OP:
        case NODE_LT:
        case NODE_GT:
        case NODE_LE:
        case NODE_GE:
        case NODE_EQ:
        case NODE_NE:
            p = codeGen(t->left, target_file);
            q = codeGen(t->right, target_file);
            
            if (strcmp(t->op, "+") == 0) fprintf(target_file, "ADD R%d, R%d\n", p, q);
            else if (strcmp(t->op, "-") == 0) fprintf(target_file, "SUB R%d, R%d\n", p, q);
            else if (strcmp(t->op, "*") == 0) fprintf(target_file, "MUL R%d, R%d\n", p, q);
            else if (strcmp(t->op, "/") == 0) fprintf(target_file, "DIV R%d, R%d\n", p, q);
            else if (strcmp(t->op, "%") == 0) fprintf(target_file, "MOD R%d, R%d\n", p, q);
            else if (strcmp(t->op, "<") == 0) fprintf(target_file, "LT R%d, R%d\n", p, q);
            else if (strcmp(t->op, ">") == 0) fprintf(target_file, "GT R%d, R%d\n", p, q);
            else if (strcmp(t->op, "<=") == 0) fprintf(target_file, "LE R%d, R%d\n", p, q);
            else if (strcmp(t->op, ">=") == 0) fprintf(target_file, "GE R%d, R%d\n", p, q);
            else if (strcmp(t->op, "==") == 0) fprintf(target_file, "EQ R%d, R%d\n", p, q);
            else if (strcmp(t->op, "!=") == 0) fprintf(target_file, "NE R%d, R%d\n", p, q);
            
            freeReg(); 
            return p;

        case NODE_IF:
            label_1 = getLabel();
            p = codeGen(t->left, target_file);
            fprintf(target_file, "JZ R%d, L%d\n", p, label_1);
            freeReg();
            codeGen(t->mid, target_file);
            if (t->right != NULL) {
                label_2 = getLabel();
                fprintf(target_file, "JMP L%d\n", label_2);
                fprintf(target_file, "L%d:\n", label_1);
                codeGen(t->right, target_file);
                fprintf(target_file, "L%d:\n", label_2);
            } else {
                fprintf(target_file, "L%d:\n", label_1);
            }
            return -1;

        case NODE_WHILE:
            label_1 = getLabel(); 
            label_2 = getLabel(); 
            while_top++;
            while_start_stack[while_top] = label_1;
            while_end_stack[while_top] = label_2;
            fprintf(target_file, "L%d:\n", label_1); 
            p = codeGen(t->left, target_file);       
            fprintf(target_file, "JZ R%d, L%d\n", p, label_2); 
            freeReg();
            codeGen(t->mid, target_file);
            fprintf(target_file, "JMP L%d\n", label_1);
            fprintf(target_file, "L%d:\n", label_2);
            while_top--;
            return -1;

        case NODE_BREAK:
            if (while_top >= 0) fprintf(target_file, "JMP L%d\n", while_end_stack[while_top]);
            return -1;
            
        case NODE_CONTINUE:
            if (while_top >= 0) fprintf(target_file, "JMP L%d\n", while_start_stack[while_top]);
            return -1;

        case NODE_FUNCALL: {

            int current_reg = reg_counter;
            for (int i = 0; i <= current_reg; i++) {
                fprintf(target_file, "PUSH R%d\n", i);
            }
            
          
            struct tnode* arg = t->arglist;
            int arg_count = 0;
            int arg_regs[20];
            while(arg != NULL) {
                arg_regs[arg_count++] = codeGen(arg, target_file);
                arg = arg->argnext;
            }
            

            for (int i = arg_count - 1; i >= 0; i--) {
                fprintf(target_file, "PUSH R%d\n", arg_regs[i]);
                freeReg();
            }
            

            temp = getReg();
            fprintf(target_file, "PUSH R%d\n", temp);
            freeReg();
            

            fprintf(target_file, "CALL F%d\n", t->Gentry->flabel);
            
          
            int res_reg = getReg();
            fprintf(target_file, "POP R%d\n", res_reg);
            

            temp = getReg();
            for (int i = 0; i < arg_count; i++) {
                fprintf(target_file, "POP R%d\n", temp);
            }
            freeReg();
            

            for (int i = current_reg; i >= 0; i--) {
                fprintf(target_file, "POP R%d\n", i);
            }
            reg_counter = current_reg + 1; // Preserve the result register
            return res_reg;
        }

        case NODE_METHODCALL: {
            int current_reg = reg_counter;
            for (int i = 0; i <= current_reg; i++)
                fprintf(target_file, "PUSH R%d\n", i);

            /* self is pushed first; the explicit arguments follow it on the stack. */
            int self_reg = codeGen(t->left, target_file);
            fprintf(target_file, "PUSH R%d\n", self_reg);
            freeReg();

            struct tnode *arg = t->arglist;
            int arg_count = 0;
            int arg_regs[32];
            while (arg) {
                if (arg_count >= 32) {
                    fprintf(stderr, "Code generation error: too many method arguments.\n");
                    exit(1);
                }
                arg_regs[arg_count++] = codeGen(arg, target_file);
                arg = arg->argnext;
            }
            for (int i = arg_count - 1; i >= 0; i--) {
                fprintf(target_file, "PUSH R%d\n", arg_regs[i]);
                freeReg();
            }

            int slot = getReg();
            fprintf(target_file, "PUSH R%d\n", slot);
            freeReg();
            fprintf(target_file, "CALL F%d\n", t->Mentry->flabel);

            int result = getReg();
            fprintf(target_file, "POP R%d\n", result);
            int discard = getReg();
            for (int i = 0; i < arg_count; i++)
                fprintf(target_file, "POP R%d\n", discard);
            /* The implicit self argument was pushed before explicit arguments. */
            fprintf(target_file, "POP R%d\n", discard);
            freeReg();
            for (int i = current_reg; i >= 0; i--)
                fprintf(target_file, "POP R%d\n", i);
            reg_counter = current_reg + 1;
            return result;
        }

        case NODE_RET: {

            p = codeGen(t->left, target_file);
            

            int addr = getReg();
            fprintf(target_file, "MOV R%d, BP\n", addr);
            int two = getReg();
            fprintf(target_file, "MOV R%d, 2\n", two);
            fprintf(target_file, "SUB R%d, R%d\n", addr, two);
            fprintf(target_file, "MOV [R%d], R%d\n", addr, p);
            freeReg(); freeReg(); freeReg();
            

            int local_count = local_binding - 1;
            temp = getReg();
            for(int i = 0; i < local_count; i++){
                fprintf(target_file, "POP R%d\n", temp);
            }
            freeReg();
            

            fprintf(target_file, "POP BP\n");
            fprintf(target_file, "RET\n");
            return -1;
        }
    }
    return -1;
}

void generateFuncCode(char* name, struct tnode* body, FILE* target_file) {
    struct Gsymbol* g = GLookUp(name);
    
    fprintf(target_file, "F%d:\n", g->flabel);
    
    fprintf(target_file, "PUSH BP\n");
    fprintf(target_file, "MOV BP, SP\n");
    
    int local_count = local_binding - 1;
    int r = getReg();
    fprintf(target_file, "MOV R%d, 0\n", r);
    for (int i = 0; i < local_count; i++) {
        fprintf(target_file, "PUSH R%d\n", r); // Initialize local slots to zero/NULL
    }
    freeReg();
    
    codeGen(body, target_file);
}

void generateMethodCode(struct Memberfunclist *method, struct tnode *body, FILE *target_file) {
    if (!method) {
        fprintf(stderr, "Code generation error: missing method table entry.\n");
        exit(1);
    }
    fprintf(target_file, "F%d:\n", method->flabel);
    fprintf(target_file, "PUSH BP\n");
    fprintf(target_file, "MOV BP, SP\n");
    int local_count = local_binding - 1;
    int r = getReg();
    fprintf(target_file, "MOV R%d, 0\n", r);
    for (int i = 0; i < local_count; i++)
        fprintf(target_file, "PUSH R%d\n", r);
    freeReg();
    codeGen(body, target_file);
}