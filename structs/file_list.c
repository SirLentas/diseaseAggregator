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
#include "file_list.h"

struct node *insert(struct node *head,long int data, char *path)
{
    //create a link
    struct node *link = (struct node *)malloc(sizeof(struct node));

    //link->key = key;
    
    link->int_date = data;
    while(link->int_date<data){
        (link->int_date)++;
    }
    // printf("%ld --- %ld\n",data,link->int_date);
    strcpy(link->full_path, path);

    if (head == NULL)
    {

        link->next = NULL;
        return link;
    }
    else if (head->int_date > link->int_date)
    {
        link->next = head;
        return link;
    }
    else
    {
        struct node *sherlock;
        sherlock = head;
        while (sherlock->next != NULL && sherlock->next->int_date < link->int_date)
        {
            sherlock = sherlock->next;
        }
        link->next = sherlock->next;
        sherlock->next = link;
        return head;
    }
}