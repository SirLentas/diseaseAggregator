struct node
{
    char full_path[30];
    long int int_date;
    struct node *next;
};

struct node *insert(struct node *head, long int data, char *path);
