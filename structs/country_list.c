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
#include "country_list.h"

struct country *c_ins(struct country *head, char *dis)
{
    //create a link
    struct country *link = (struct country *)malloc(sizeof(struct country));

    //link->key = key;
    
    strcpy(link->cntr, dis);
    link->next=NULL;

    if (head == NULL)
    {
        return link;
    }
    struct country *sherlock = head;
    while (sherlock->next != NULL)
    {
        sherlock = sherlock->next;
    }
    sherlock->next = link;
    return head;
}