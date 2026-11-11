#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char n[32];
    int a;
} Lab;

int main(int c, char **v)
{
    if (c != 3)
        return 1;

    FILE *i = fopen(v[1], "r");
    FILE *o = fopen(v[2], "w");

    char s[256];
    Lab L[1024];

    int n = 0;
    int a = 2056;

    if (!i || !o)
        return 1;

    /* Copy 8-line XSM header */
    for (int k = 0; k < 8; k++)
    {
        fgets(s, 256, i);
        fputs(s, o);
    }

    /* PASS 1: find labels and their addresses */
    while (fgets(s, 256, i))
    {
        char *colon = strchr(s, ':');

        if (colon)
        {
            *colon = '\0';

            strcpy(L[n].n, s);
            L[n].a = a;
            n++;
        }
        else if (s[0] != '\n')
        {
            a += 2;
        }
    }

    /* Go back to beginning */
    rewind(i);

    /* Skip header */
    for (int k = 0; k < 8; k++)
        fgets(s, 256, i);

    /* PASS 2: replace labels with addresses */
    while (fgets(s, 256, i))
    {
        char op[32], label[32];
        
        /* Label declaration: don't copy it to output */
        if (strchr(s, ':'))
            continue;

        /*
        Labels occur only as operands of JMP/JZ/JNZ/CALL.
        Parse the instruction instead of doing substring replacement.
        */
        if (sscanf(s, "%31s %*[^,], %31s", op, label) == 2)
        {
            /* Remove trailing newline */
            label[strcspn(label, "\r\n")] = '\0';

            for (int j = 0; j < n; j++)
            {
                if (strcmp(label, L[j].n) == 0)
                {
                    char *comma = strrchr(s, ',');
                    
                    if (comma)
                    {
                        /* Preserve everything before operand */
                        char prefix[256];
                        size_t len = comma - s + 1;

                        strncpy(prefix, s, len);
                        prefix[len] = '\0';

                        snprintf(s, 256, "%s%d\n",
                                prefix, L[j].a);
                    }

                    break;
                }
            }
        }
        else
        {
            /* JMP Lx / CALL Lx */
            if (sscanf(s, "%31s %31s", op, label) == 2)
            {
                label[strcspn(label, "\r\n")] = '\0';

                for (int j = 0; j < n; j++)
                {
                    if (strcmp(label, L[j].n) == 0)
                    {
                        snprintf(s, 256, "%s %d\n", op, L[j].a);
                        break;
                    }
                }
            }
        }

        fputs(s, o);
    }

    fclose(i);
    fclose(o);

    return 0;
}