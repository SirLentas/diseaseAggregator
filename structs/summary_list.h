struct summary
{
    char dis[20];
    char cntr[25];
    long int int_date;
    int zero;
    int twenty;
    int forty;
    int sixty;
    struct summary *next;
};

int update(struct summary *head, int age, char *dis, char *country,long int date);
struct summary *ins(struct summary *head, int age, char *dis, char *country,long int date);
struct summary *ins2(struct summary *head,long int date, char *country, char *dis, int z, int t, int f, int s);