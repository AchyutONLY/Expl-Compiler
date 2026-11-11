#include <stdio.h>

int main()
{
    FILE *target_file;

    target_file = fopen("target.xsm", "w");

    if (target_file == NULL)
    {
        printf("Error opening file!\n");
        return 1;
    }

    fprintf(target_file, "0\n");
    fprintf(target_file, "2056\n");
    fprintf(target_file, "0\n");
    fprintf(target_file, "0\n");
    fprintf(target_file, "0\n");
    fprintf(target_file, "0\n");
    fprintf(target_file, "0\n");
    fprintf(target_file, "0\n");


    fprintf(target_file, "BRKP\n");
    fprintf(target_file, "MOV R0,10\n");
    fprintf(target_file, "MOV R1,3\n");
    fprintf(target_file, "MOV R2,5\n");
    fprintf(target_file, "MOV R3,R0\n");
    fprintf(target_file, "MOV R4,R1\n");
    fprintf(target_file, "GT R4,R3\n");
    fprintf(target_file, "JZ R4,2074\n");
    fprintf(target_file, "MOV R3,R1\n");
    fprintf(target_file, "MOV R4,R2\n");
    fprintf(target_file, "GT R4,R3\n");
    fprintf(target_file, "JZ R4,2082\n");
    fprintf(target_file, "MOV R3,R2\n");
    fprintf(target_file, "MOV [4096],R3\n");
    fprintf(target_file, "HALT\n");

    fclose(target_file);

    printf("target.xsm generated successfully.\n");

    return 0;
}
