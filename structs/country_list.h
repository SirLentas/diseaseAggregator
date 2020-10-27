struct country
{
    char cntr[20];
    struct country *next;
};

struct country *c_ins(struct country *head, char *path);
