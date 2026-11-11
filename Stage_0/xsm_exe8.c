#include <stdio.h>

int main()
{
    FILE *fp = fopen("target.xsm", "w");

    if (fp == NULL)
    {
        printf("Error opening target.xsm\n");
        return 1;
    }

    fprintf(fp, "0\n");
    fprintf(fp, "2056\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");
    fprintf(fp, "0\n");

    fprintf(fp, "BRKP\n");
    fprintf(fp, "MOV SP, 4096\n");
    fprintf(fp, "MOV R0, 0\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "CALL 2116\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "MOV R1, [4096]\n");
    fprintf(fp, "JZ R1, 2076\n");
    fprintf(fp, "ADD R0, R1\n");
    fprintf(fp, "JMP 2062\n");

    fprintf(fp, "MOV R2, \"Write\"\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "MOV R2, -2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "CALL 0\n");

    fprintf(fp, "POP R0\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R1\n");

    fprintf(fp, "MOV R2, \"Exit\"\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "CALL 0\n");

    fprintf(fp, "MOV R10, \"Read\"\n");
    fprintf(fp, "PUSH R10\n");
    fprintf(fp, "MOV R10, -1\n");
    fprintf(fp, "PUSH R10\n");
    fprintf(fp, "MOV R10, 4096\n");
    fprintf(fp, "PUSH R10\n");
    fprintf(fp, "PUSH R10\n");
    fprintf(fp, "PUSH R10\n");
    fprintf(fp, "CALL 0\n");
    fprintf(fp, "POP R10\n");
    fprintf(fp, "POP R10\n");
    fprintf(fp, "POP R10\n");
    fprintf(fp, "POP R10\n");
    fprintf(fp, "POP R10\n");
    fprintf(fp, "RET\n");

    fclose(fp);

    printf("target.xsm generated successfully.\n");

    return 0;
}
