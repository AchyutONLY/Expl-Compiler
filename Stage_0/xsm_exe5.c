#include <stdio.h>

int main()
{
    FILE *fp = fopen("target.xsm", "w");

    if (fp == NULL)
    {
        printf("Error opening target.xsm\n");
        return 1;
    }

    /* Header */
    fprintf(fp, "0\n");
    fprintf(fp, "2056\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");

    fprintf(fp, "BRKP\n");
    fprintf(fp, "MOV R0, 0\n");
    fprintf(fp, "MOV SP, 4096\n");
    fprintf(fp, "CALL 2112\n");
    fprintf(fp, "MOV R1, [4096]\n");
    fprintf(fp, "ADD R0, R1\n");
    fprintf(fp, "JZ R1, 2072\n");
    fprintf(fp, "JMP 2062\n");

    fprintf(fp, "MOV R2, 5\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "MOV R2, -2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "INT 7\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R1\n");

    fprintf(fp, "MOV R2, 10\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "INT 10\n");

    fprintf(fp, "MOV R2, 5\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "MOV R2, -2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "MOV R2, 4096\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "INT 6\n");
    fprintf(fp, "POP R2\n");
    fprintf(fp, "POP R2\n");
    fprintf(fp, "POP R2\n");
    fprintf(fp, "POP R2\n");
    fprintf(fp, "POP R2\n");
    fprintf(fp, "RET\n");

    fclose(fp);

    printf("target.xsm generated successfully.\n");

    return 0;
}
