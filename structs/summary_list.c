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
#include "summary_list.h"

int update(struct summary *head, int age, char *dis, char *country,long int date)
{
    //create a link
    struct summary *sherlock;
    sherlock = head;
    while (sherlock != NULL)
    {
        if (strcmp(sherlock->dis, dis) == 0 && strcmp(sherlock->cntr, country) == 0 && sherlock->int_date == date)
        {
            if (age <= 20)
            {
                (sherlock->zero)++;
            }
            else if (age <= 40)
            {
                (sherlock->twenty)++;
            }
            else if (age <= 60)
            {
                (sherlock->forty)++;
            }
            else
            {
                (sherlock->sixty)++;
            }
            return 1;
        }
        sherlock = sherlock->next;
    }
    return 0;
}

struct summary *ins(struct summary *head, int age, char *dis, char *country,long int date)
{
    struct summary *link = (struct summary *)malloc(sizeof(struct summary));

    //link->key = key;
    link->zero = 0;
    link->twenty = 0;
    link->forty = 0;
    link->sixty = 0;
    if (age <= 20)
    {
        (link->zero)++;
    }
    else if (age <= 40)
    {
        (link->twenty)++;
    }
    else if (age <= 60)
    {
        (link->forty)++;
    }
    else
    {
        (link->sixty)++;
    }
    link->int_date = date;
    strcpy(link->dis, dis);
    strcpy(link->cntr, country);
    link->next = NULL;
    if (head == NULL)
    {
        return link;
    }
    struct summary *sherlock = head;
    while (sherlock->next != NULL)
    {
        sherlock = sherlock->next;
    }
    sherlock->next = link;
    return head;
}

struct summary *ins2(struct summary *head,long int date, char *country, char *dis, int z, int t, int f, int s)
{
    struct summary *newS = (struct summary *)malloc(sizeof(struct summary));
    newS->int_date = date;
    strcpy(newS->dis, dis);
    strcpy(newS->cntr, country);
    newS->zero=z;
    newS->twenty=t;
    newS->forty=f;
    newS->sixty=s;
    newS->next = NULL;

    if (head == NULL)
    {
        return newS;
    }
    struct summary *sherlock = head;
    while (sherlock->next != NULL)
    {
        sherlock = sherlock->next;
    }
    sherlock->next = newS;
    return head;
}