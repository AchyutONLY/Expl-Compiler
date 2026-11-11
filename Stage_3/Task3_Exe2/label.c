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
        if (strchr(s, ':'))
            continue;

        for (int j = 0; j < n; j++)
        {
            char *x = strstr(s, L[j].n);

            if (x)
            {
                char tail[200];
                char out[256];

                strcpy(tail, x + strlen(L[j].n));

                *x = '\0';

                snprintf(out, 256, "%s%d%s",
                         s,
                         L[j].a,
                         tail);

                strcpy(s, out);
            }
        }

        fputs(s, o);
    }

    fclose(i);
    fclose(o);

    return 0;
}