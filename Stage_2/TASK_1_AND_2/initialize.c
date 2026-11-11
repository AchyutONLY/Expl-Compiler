extern FILE * target;

void initialize()
{
    target=fopen("target.xsm","w");
    fprintf(target,"0\n2056\n0\n0\n0\n0\n0\n0\n");
    fprintf(target,"ADD SP, 26\n");
}