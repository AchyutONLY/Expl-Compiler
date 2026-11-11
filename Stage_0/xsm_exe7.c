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
    fprintf(fp, "CALL 2152\n");
    fprintf(fp, "MOV R0, [4096]\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "CALL 2152\n");
    fprintf(fp, "MOV R0, [4096]\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "CALL 2152\n");
    fprintf(fp, "MOV R2, [4096]\n");
    fprintf(fp, "POP R1\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "MOV R6, R0\n");
    fprintf(fp, "LT R6, R1\n");
    fprintf(fp, "JZ R6, 2100\n");
    fprintf(fp, "MOV R6, R1\n");
    fprintf(fp, "LT R6, R2\n");
    fprintf(fp, "JZ R6, 2096\n");
    fprintf(fp, "MOV R7, R2\n");
    fprintf(fp, "JMP 2112\n");
    fprintf(fp, "MOV R7, R1\n");
    fprintf(fp, "JMP 2112\n");
    fprintf(fp, "MOV R6, R0\n");
    fprintf(fp, "LT R6, R2\n");
    fprintf(fp, "JZ R6, 2110\n");
    fprintf(fp, "MOV R7, R2\n");
    fprintf(fp, "JMP 2112\n");
    fprintf(fp, "MOV R7, R0\n");

    fprintf(fp, "MOV R2, \"Write\"\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "MOV R2, -2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R7\n");
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
