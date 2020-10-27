#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "records.h"

struct rec *rec_ins(struct rec *head, char *id, char *fn, char *ln, int age, long int inDate, char *dis, char* coun)
{
    struct rec *link = (struct rec *)malloc(sizeof(struct rec));

    //link->key = key;

    strcpy(link->disease, dis);
    strcpy(link->country, coun);
    strcpy(link->first_name, fn);
    strcpy(link->last_name, ln);
    strcpy(link->ID, id);
    link->inDate = inDate;
    link->age = age;
    link->outDate = 0;
    link->next = NULL;

    if (head == NULL)
    {
        return link;
    }
    struct rec *sherlock = head;
    while (sherlock->next != NULL)
    {
        sherlock = sherlock->next;
    }
    sherlock->next = link;
    return head;
}

int new_exit(struct rec *head, char *id, long int outDate)
{
    struct rec *sherlock = head;
    while (sherlock != NULL)
    {
        if (strcmp(sherlock->ID, id) == 0)
        {
            if (outDate <= sherlock->inDate)
            {
                return -1;
            }
            sherlock->outDate = outDate;
            return 1;
        }
        sherlock = sherlock->next;
    }
    return 0;
}

// int printRec(struct rec *head, char *id)
// {
//     struct rec *sherlock = head;
//     while (sherlock != NULL)
//     {
//         if (strcmp(sherlock->ID, id) == 0 && sherlock->outDate==0)
//         {
//             printf("%s %s %s %s %s %d %d-%d-%d -\n",);
//             return 1;
//         }else if(strcmp(sherlock->ID, id) == 0){
//             printf("%s %s %s %s %s %d %d-%d-%d %d-%d-%d\n");
//             return 1;
//         }
//         sherlock = sherlock->next;
//     }
//     return 0;
// }