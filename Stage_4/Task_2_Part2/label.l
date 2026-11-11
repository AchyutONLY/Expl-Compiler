%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 2048
#define MAX_LABELS 1024
#define HEADER_LINES 8
#define START_ADDRESS 2056

typedef struct
{
    char name[32];
    int address;
} Label;

char lines[MAX_LINES][256];
int lineCount = 0;

Label labels[MAX_LABELS];
int labelCount = 0;

char currentLine[256];
int currentPos = 0;

int findLabel(char *name)
{
    int i;

    for(i = 0; i < labelCount; i++)
    {
        if(strcmp(labels[i].name, name) == 0)
        {
            return labels[i].address;
        }
    }

    return -1;
}

void addLabel(char *name, int address)
{
    if(labelCount >= MAX_LABELS)
    {
        printf("Too many labels\n");
        exit(1);
    }

    strcpy(labels[labelCount].name, name);
    labels[labelCount].address = address;
    labelCount++;
}

void storeLine()
{
    if(lineCount >= MAX_LINES)
    {
        printf("Too many lines\n");
        exit(1);
    }

    currentLine[currentPos] = '\0';

    strcpy(lines[lineCount], currentLine);

    lineCount++;
    currentPos = 0;
}

void pass1()
{
    int address = START_ADDRESS;
    int i;

    for(i = HEADER_LINES; i < lineCount; i++)
    {
        char temp[256];
        char name[32];
        char *colon;

        strcpy(temp, lines[i]);

        colon = strchr(temp, ':');

        if(colon != NULL)
        {
            *colon = '\0';

            if(sscanf(temp, "%31s", name) == 1)
            {
                addLabel(name, address);
            }
        }
        else if(temp[0] != '\0')
        {
            address = address + 2;
        }
    }
}

void pass2(FILE *out)
{
    int i;

    /* Copy the 8-line XSM header */
    for(i = 0; i < HEADER_LINES; i++)
    {
        fprintf(out, "%s\n", lines[i]);
    }

    for(i = HEADER_LINES; i < lineCount; i++)
    {
        char line[256];

        char op[32];
        char arg1[64];
        char arg2[64];

        strcpy(line, lines[i]);

        /*
         * Remove label declarations.
         */
        if(strchr(line, ':') != NULL)
        {
            continue;
        }

        /*
         * JMP LABEL
         * CALL LABEL
         */
        if(sscanf(line, "%31s %63s", op, arg1) == 2)
        {
            int target;

            if(strcmp(op, "JMP") == 0 ||
               strcmp(op, "CALL") == 0)
            {
                target = findLabel(arg1);

                if(target != -1)
                {
                    fprintf(out, "%s %d\n", op, target);
                    continue;
                }
            }
        }

        /*
         * JZ R0, LABEL
         * JNZ R0, LABEL
         */
        if(sscanf(line, "%31s %63[^,], %63s",
                  op, arg1, arg2) == 3)
        {
            int target;

            if(strcmp(op, "JZ") == 0 ||
               strcmp(op, "JNZ") == 0)
            {
                target = findLabel(arg2);

                if(target != -1)
                {
                    fprintf(out,
                            "%s %s, %d\n",
                            op,
                            arg1,
                            target);

                    continue;
                }
            }
        }

        /*
         * Normal instruction.
         */
        fprintf(out, "%s\n", line);
    }
}

%}

/* ----------- LEX RULES ----------- */

%%

\n    {
        storeLine();
    }

.     {
        if(currentPos >= 255)
        {
            printf("Line too long\n");
            exit(1);
        }

        currentLine[currentPos] = yytext[0];
        currentPos++;
    }

%%

int yywrap()
{
    return 1;
}

int main(int argc, char **argv)
{
    FILE *input;
    FILE *output;

    if(argc != 3)
    {
        printf("Usage: %s input.xsm output.xsm\n", argv[0]);
        return 1;
    }

    input = fopen(argv[1], "r");

    if(input == NULL)
    {
        printf("Cannot open input file\n");
        return 1;
    }

    /*
     * Lex scans the complete target file once.
     */
    yyin = input;
    yylex();

    fclose(input);

    /*
     * Last line may not have a newline.
     */
    if(currentPos > 0)
    {
        storeLine();
    }

    if(lineCount < HEADER_LINES)
    {
        printf("Invalid XSM file\n");
        return 1;
    }

    /*
     * PASS 1
     */
    pass1();

    /*
     * PASS 2
     */
    output = fopen(argv[2], "w");

    if(output == NULL)
    {
        printf("Cannot create output file\n");
        return 1;
    }

    pass2(output);

    fclose(output);

    return 0;
}