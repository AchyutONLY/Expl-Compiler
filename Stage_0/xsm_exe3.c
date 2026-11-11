#include <stdio.h>

int main()
{
    FILE *fp = fopen("target.xsm", "w");

    if (fp == NULL)
    {
        printf("Error creating target file.\n");
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
    fprintf(fp, "MOV R0, 3\n");
    fprintf(fp, "MOV R1, 2\n");
    fprintf(fp, "ADD R0, R1\n");
    fprintf(fp, "MOV SP, 4095\n");
    fprintf(fp, "MOV R2, 5\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "MOV R2, -2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R0\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "INT 7\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "POP R0\n");
    fprintf(fp, "MOV R2, 0\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "PUSH R2\n");
    fprintf(fp, "INT 10\n");
    fclose(fp);
    printf("Target file generated successfully.\n");
    return 0;
}
