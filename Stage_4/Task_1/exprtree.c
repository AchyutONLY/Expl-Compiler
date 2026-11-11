

Gsymbol *Ghead = NULL;
int nextBinding = 4096;

Gsymbol *Lookup(char *name)
{
    Gsymbol *temp = Ghead;

    while (temp != NULL)
    {
        if (strcmp(temp->name, name) == 0)
            return temp;
        temp = temp->next;
    }

    return NULL;
}

void Install(char *name, int type, int size)
{
    if (Lookup(name) != NULL)
    {
        fprintf(stderr, "Error: variable '%s' declared multiple times\n", name);
        exit(1);
    }

    Gsymbol *newnode = (Gsymbol *)malloc(sizeof(Gsymbol));
    if (newnode == NULL)
    {
        perror("malloc");
        exit(1);
    }

    newnode->name = strdup(name);
    newnode->type = type;
    newnode->size = size;
    newnode->binding = nextBinding;
    newnode->next = NULL;

    if (Ghead == NULL)
    {
        Ghead = newnode;
    }
    else
    {
        Gsymbol *temp = Ghead;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = newnode;
    }

    nextBinding += size;
}

void PrintGsymbolTable(void)
{
    Gsymbol *temp = Ghead;

    printf("\nGlobal Symbol Table\n");
    printf("%-15s %-10s %-10s %-10s\n",
           "Name", "Type", "Size", "Binding");
    printf("-----------------------------------------------\n");

    while (temp != NULL)
    {
        printf("%-15s %-10s %-10d %-10d\n",
               temp->name,
               (temp->type == TYPE_INT) ? "INT" : "STR",
               temp->size,
               temp->binding);
        temp = temp->next;
    }
}