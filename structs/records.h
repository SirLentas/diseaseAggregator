struct rec
{
    char first_name[20];
    char last_name[20];
    int age;
    long int inDate;
    long int outDate;
    char ID[20];
    char country[20];
    char disease[20];

    struct rec *next;
};

struct rec *rec_ins(struct rec *head, char *id, char *fn, char *ln, int age, long int inDate, char *dis,char* coun);
int new_exit(struct rec *head, char *id, long int outDate);
// int printRec(struct rec *head, char *id);
