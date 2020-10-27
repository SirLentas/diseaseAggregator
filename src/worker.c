#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../structs/file_list.h"
#include "../structs/summary_list.h"
#include "../structs/records.h"

long int datetoint(const char *s);
void intHandlerChild() {}
int refresh = 0;
int ppid;
void refHandlerChild();

int main(int argc, char *argv[]) //./worker fifo_in fifo_out workload parent_dir buffersize
{
    // printf("{%d} ./worker 1.%s 2.%s 3.%s 4.%s 5.%s\n", getpid(), argv[1], argv[2], argv[3], argv[4], argv[5]);
    signal(SIGINT, intHandlerChild);
    signal(SIGQUIT, intHandlerChild);
    signal(SIGUSR2, refHandlerChild);
    // printf("!!!!\n");

    ppid = getppid();

    char err[35];
    sprintf(err, "./error_reports/error_report.%d", getpid());
    FILE *er = fopen(err, "a+"); //anoigei ena arxeio opou ektypwnei kathe error i kathe success kata thn anagnwsh twn eggrafwn

    int workload = atoi(argv[3]);
    int buffersize = atoi(argv[5]);

    int requests = 0;
    int req_done = 0;
    int req_failed = 0;

    char mid_path[workload][40];
    char countries_served[workload][30];
    long int last_date[workload];
    int ok = 1;
    int in_fd, out_fd;

    struct node *head[workload];
    struct rec *records_head = NULL;

    for (int i = 0; i < workload; i++) //gia kathe xwra poy tou exei antethei
    {
        head[i] = NULL;
        last_date[i] = 0;
        int chunks = 0;
        char cntr[30];

        in_fd = open(argv[1], O_RDONLY);
        while (in_fd < 0)
        {
            in_fd = open(argv[1], O_RDONLY);
        }
        read(in_fd, &chunks, sizeof(int));
        close(in_fd);

        for (int c = 0; c < chunks; c++)
        {
            char *read_chunk = malloc(buffersize + 1);
            out_fd = open(argv[2], O_WRONLY);
            while (out_fd < 0)
            {
                out_fd = open(argv[2], O_WRONLY);
            }
            write(out_fd, &ok, sizeof(int));
            close(out_fd);

            in_fd = open(argv[1], O_RDONLY);
            while (in_fd < 0)
            {
                in_fd = open(argv[1], O_RDONLY);
            }
            read(in_fd, read_chunk, buffersize);

            read_chunk[buffersize] = '\0';

            close(in_fd);

            if (c == 0)
            {
                strcpy(cntr, read_chunk);
            }
            else
            {

                strcat(cntr, read_chunk);
            }
            free(read_chunk);
        }

        strcpy(mid_path[i], argv[4]); //dhmioyrgei to path gia toys fakelous poy tha anoiksei
        strcat(mid_path[i], cntr);
        strcat(mid_path[i], "/");

        strcpy(countries_served[i], cntr);

        DIR *cntr_folder = opendir(mid_path[i]);
        struct dirent *date_folder;

        if (cntr_folder == NULL)
        {
            fprintf(er, "Could not open current directory\n");
            return 0;
        }

        while ((date_folder = readdir(cntr_folder)) != NULL) //bazei se mia lista olous toys ypofakelous-hmeromhnies se ayjousa seira
        {
            if (strcmp(date_folder->d_name, ".") != 0 && strcmp(date_folder->d_name, "..") != 0)
            {
                char full_path[100];
                strcpy(full_path, mid_path[i]);
                strcat(full_path, date_folder->d_name);

                head[i] = insert(head[i], datetoint(date_folder->d_name), full_path);
                if (datetoint(date_folder->d_name) > last_date[i])
                {
                    last_date[i] = datetoint(date_folder->d_name);
                }
            }
        }

        // struct node *sherlock = head[i];

        struct node *ptr = head[i]; //deikths sto head ths listas me tiw hmeromhnies
        struct summary *sums = NULL;
        int k = 0;

        char *key;
        char *type;
        char *fn;
        char *ln;
        char *dis;
        int age = 0;
        long int inDate = 0;
        while (ptr != NULL) //oso yparxoyn kai alloi fakeloi
        {

            FILE *infile = fopen(ptr->full_path, "r");

            if (infile != NULL)
            {
                char line[120];
                while (fgets(line, sizeof(line), infile))
                {
                    key = strtok(line, " ");
                    type = strtok(NULL, " ");
                    fn = strtok(NULL, " ");
                    ln = strtok(NULL, " ");
                    dis = strtok(NULL, " ");
                    age = atoi(strtok(NULL, "\n"));

                    if (strcmp(type, "ENTER") == 0)
                    {
                        records_head = rec_ins(records_head, key, fn, ln, age, ptr->int_date, dis, countries_served[i]); //eisagei mia nea rec sth lista eggrafwn
                        fprintf(er, "New Record with ID: %s added\n", key);                                              //typwnei mymnhma sto error_report.(pid)
                        int done = update(sums, age, dis, cntr, ptr->int_date);                                          //psaxnei sta yparxonta sums kai ananevnei ta kroysmata ana kathgoria
                        if (done == 0)                                                                                   //an den vrei sum gia ayth thn arrvstia se ayth thn hmeromhnia epistrefei 0
                        {
                            sums = ins(sums, age, dis, cntr, ptr->int_date); //eisagei ena neo sum sth lista
                            k++;
                        }
                    }
                    else if (strcmp(type, "EXIT") == 0)
                    {

                        int err_code = new_exit(records_head, key, ptr->int_date); //an einai typou exit prospathei na ananewsei ta rec
                        if (err_code == 0)                                         //analoga th timh pou epistrefete ektypwnei sto error_report katallhlo mynhma
                        {
                            fprintf(er, "ERROR not existing record with ID: %s\n", key);
                        }
                        else if (err_code == -1)
                        {
                            fprintf(er, "ERROR no valid exit date for record with ID: %s\n", key);
                        }
                        else
                        {
                            fprintf(er, "Exit date updated for record with ID: %s\n", key);
                        }
                    }
                }

                fclose(infile);
            }
            else
            {

                fprintf(er, "Unable to open file\n");
            }

            ptr = ptr->next;
        }

        out_fd = open(argv[2], O_WRONLY);
        while (out_fd < 0)
        {
            out_fd = open(argv[2], O_WRONLY);
        }
        write(out_fd, &k, sizeof(int)); //grafei sto pipe posa sums tha epistrepsei
        close(out_fd);

        struct summary *holmes = sums;
        for (int s = 0; s < k; s++)
        {

            int ok;
            char *sample = malloc(200 * sizeof(char));
            sprintf(sample, "%ld-%ld-%ld/%s/%s/%d/%d/%d/%d|\n", ((holmes->int_date) % 360) % 30, ((holmes->int_date) % 360) / 30, (holmes->int_date) / 360, holmes->cntr, holmes->dis, holmes->zero, holmes->twenty, holmes->forty, holmes->sixty);

            // char *str = malloc(200 * sizeof(char));
            // strcpy(str, sample);

            int s_length = strlen(sample); //xwrizei kathe summary se chunks
            int sum_chunks = s_length / buffersize;
            if ((s_length % buffersize) > 0)
            {
                sum_chunks++;
            }

            in_fd = open(argv[1], O_RDONLY);
            while (in_fd < 0)
            {
                in_fd = open(argv[2], O_RDONLY);
            }
            read(in_fd, &ok, sizeof(int));
            close(in_fd);

            out_fd = open(argv[2], O_WRONLY);
            while (out_fd < 0)
            {
                out_fd = open(argv[2], O_WRONLY);
            }
            write(out_fd, &sum_chunks, sizeof(int));
            close(out_fd);

            for (int sc = 0; sc < sum_chunks; sc++)
            {
                in_fd = open(argv[1], O_RDONLY);
                while (in_fd < 0)
                {
                    in_fd = open(argv[2], O_RDONLY);
                }
                read(in_fd, &ok, sizeof(int));
                close(in_fd);

                char *str2 = malloc(sizeof(char) * buffersize);
                for (int j = 0; j < buffersize; j++)
                {
                    if ((j + sc * buffersize) < s_length)
                        str2[j] = sample[j + sc * buffersize];
                    else
                    {
                        str2[j] = '\0';
                    }
                }

                out_fd = open(argv[2], O_WRONLY);
                while (out_fd < 0)
                {
                    out_fd = open(argv[2], O_WRONLY);
                }
                write(out_fd, str2, buffersize);
                close(out_fd);
                free(str2);
            }

            holmes = holmes->next;
            free(sample);
            // free(str);
        }
        struct summary *sum_del = sums; //apeleytherwnei th lista me ta sums afotu ta steilei sto parent
        struct summary *sum_del_temp;
        for (int iam = 0; iam < k; iam++)
        {
            sum_del_temp = sum_del->next;
            free(sum_del);
            sum_del = sum_del_temp;
        }

        closedir(cntr_folder);
    }

    int done = 1;
    int read_done = 0;

    while (done)
    {
        //perimenei na diabasei apo to pipe, 0 an den xreiazetai allo, 1 an tha prepei na diavasei kapoia entolh ap to pipe

        in_fd = open(argv[1], O_RDONLY);
        while (in_fd < 0)
        {
            in_fd = open(argv[1], O_RDONLY);
        }
        read(in_fd, &read_done, sizeof(int)); //divazei
        close(in_fd);

        if (read_done == 0)
        {
            out_fd = open(argv[2], O_WRONLY);
            while (out_fd < 0)
            {
                out_fd = open(argv[2], O_WRONLY);
            }
            write(out_fd, &ok, sizeof(int)); //apantaei
            close(out_fd);
            done = 0;
        }
        else if (read_done == 2)
        {
            out_fd = open(argv[2], O_WRONLY);
            while (out_fd < 0)
            {
                out_fd = open(argv[2], O_WRONLY);
            }
            write(out_fd, &refresh, sizeof(int)); //apantaei
            close(out_fd);

            if (refresh == 1)
            {
                in_fd = open(argv[1], O_RDONLY);
                while (in_fd < 0)
                {
                    in_fd = open(argv[1], O_RDONLY);
                }
                read(in_fd, &ok, sizeof(int)); //divazei
                close(in_fd);

                int num_of_sums = 0;

                struct node *head2[workload];
                struct summary *sums2 = NULL;

                fprintf(er, "============================\n");

                for (int i = 0; i < workload; i++)
                {
                    head2[i] = NULL;
                    DIR *cntr_folder = opendir(mid_path[i]);
                    struct dirent *date_folder;

                    if (cntr_folder == NULL)
                    {
                        fprintf(er, "Could not open current directory\n");
                        return 0;
                    }

                    long int ld = 0;

                    while ((date_folder = readdir(cntr_folder)) != NULL) //bazei se mia lista olous toys ypofakelous-hmeromhnies se ayjousa seira
                    {
                        if (strcmp(date_folder->d_name, ".") != 0 && strcmp(date_folder->d_name, "..") != 0)
                        {
                            if (datetoint(date_folder->d_name) > last_date[i])
                            {
                                char full_path[100];
                                strcpy(full_path, mid_path[i]);
                                strcat(full_path, date_folder->d_name);

                                head2[i] = insert(head2[i], datetoint(date_folder->d_name), full_path);

                                if (datetoint(date_folder->d_name) > ld)
                                {

                                    ld = datetoint(date_folder->d_name);
                                }
                            }
                        }
                    }

                    closedir(cntr_folder);
                    if (ld > 0)
                    {
                        last_date[i] = ld;
                    }

                    struct node *ptr = head2[i]; //deikths sto head ths listas me tis nees hmeromhnies
                    // struct summary *sums2 = NULL;

                    char *key;
                    char *type;
                    char *fn;
                    char *ln;
                    char *dis;
                    int age = 0;
                    long int inDate = 0;
                    while (ptr != NULL) //oso yparxoyn kai alloi fakeloi
                    {

                        FILE *infile = fopen(ptr->full_path, "r");

                        if (infile != NULL)
                        {
                            char line[120];
                            while (fgets(line, sizeof(line), infile))
                            {
                                key = strtok(line, " ");
                                type = strtok(NULL, " ");
                                fn = strtok(NULL, " ");
                                ln = strtok(NULL, " ");
                                dis = strtok(NULL, " ");
                                age = atoi(strtok(NULL, "\n"));

                                if (strcmp(type, "ENTER") == 0)
                                {
                                    records_head = rec_ins(records_head, key, fn, ln, age, ptr->int_date, dis, countries_served[i]); //eisagei mia nea rec sth lista eggrafwn
                                    fprintf(er, "New Record with ID: %s added\n", key);                                              //typwnei mymnhma sto error_report.(pid)
                                    int done = update(sums2, age, dis, countries_served[i], ptr->int_date);                          //psaxnei sta yparxonta sums kai ananevnei ta kroysmata ana kathgoria
                                    if (done == 0)                                                                                   //an den vrei sum gia ayth thn arrvstia se ayth thn hmeromhnia epistrefei 0
                                    {
                                        sums2 = ins(sums2, age, dis, countries_served[i], ptr->int_date); //eisagei ena neo sum sth lista
                                        num_of_sums++;
                                    }
                                }
                                else if (strcmp(type, "EXIT") == 0)
                                {

                                    int err_code = new_exit(records_head, key, ptr->int_date); //an einai typou exit prospathei na ananewsei ta rec
                                    if (err_code == 0)                                         //analoga th timh pou epistrefete ektypwnei sto error_report katallhlo mynhma
                                    {
                                        fprintf(er, "ERROR not existing record with ID: %s\n", key);
                                    }
                                    else if (err_code == -1)
                                    {
                                        fprintf(er, "ERROR no valid exit date for record with ID: %s\n", key);
                                    }
                                    else
                                    {
                                        fprintf(er, "Exit date updated for record with ID: %s\n", key);
                                    }
                                }
                            }

                            fclose(infile);
                        }
                        else
                        {

                            fprintf(er, "Unable to open file\n");
                        }

                        ptr = ptr->next;
                    }
                    struct node *rec_del = head2[i];
                    struct node *rec_del_temp = NULL;
                    while (rec_del != NULL)
                    {
                        rec_del_temp = rec_del->next;
                        free(rec_del);
                        rec_del = rec_del_temp;
                        // rec_del = rec_sherlock->next;
                    }
                }

                out_fd = open(argv[2], O_WRONLY);
                while (out_fd < 0)
                {
                    out_fd = open(argv[2], O_WRONLY);
                }
                write(out_fd, &num_of_sums, sizeof(int)); //steleni ton arithmo twn sums
                close(out_fd);

                struct summary *holmes = sums2;
                for (int s = 0; s < num_of_sums; s++)
                {
                    int ok;
                    char *sample = malloc(200 * sizeof(char));
                    sprintf(sample, "%ld-%ld-%ld/%s/%s/%d/%d/%d/%d|\n", ((holmes->int_date) % 360) % 30, ((holmes->int_date) % 360) / 30, (holmes->int_date) / 360, holmes->cntr, holmes->dis, holmes->zero, holmes->twenty, holmes->forty, holmes->sixty);

                    // char *str = malloc(200 * sizeof(char));
                    // strcpy(str, sample);

                    int s_length = strlen(sample); //xwrizei kathe summary se chunks
                    int sum_chunks = s_length / buffersize;
                    if ((s_length % buffersize) > 0)
                    {
                        sum_chunks++;
                    }

                    in_fd = open(argv[1], O_RDONLY);
                    while (in_fd < 0)
                    {
                        in_fd = open(argv[2], O_RDONLY);
                    }
                    read(in_fd, &ok, sizeof(int));
                    close(in_fd);

                    out_fd = open(argv[2], O_WRONLY);
                    while (out_fd < 0)
                    {
                        out_fd = open(argv[2], O_WRONLY);
                    }
                    write(out_fd, &sum_chunks, sizeof(int));
                    close(out_fd);

                    for (int sc = 0; sc < sum_chunks; sc++)
                    {
                        in_fd = open(argv[1], O_RDONLY);
                        while (in_fd < 0)
                        {
                            in_fd = open(argv[2], O_RDONLY);
                        }
                        read(in_fd, &ok, sizeof(int));
                        close(in_fd);

                        char *str2 = malloc(sizeof(char) * buffersize);
                        for (int j = 0; j < buffersize; j++)
                        {
                            if ((j + sc * buffersize) < s_length)
                                str2[j] = sample[j + sc * buffersize];
                            else
                            {
                                str2[j] = '\0';
                            }
                        }

                        out_fd = open(argv[2], O_WRONLY);
                        while (out_fd < 0)
                        {
                            out_fd = open(argv[2], O_WRONLY);
                        }
                        write(out_fd, str2, buffersize);
                        close(out_fd);
                        free(str2);
                    }

                    holmes = holmes->next;
                    free(sample);
                    // free(str);
                }

                refresh = 0;
                struct summary *sum_del = sums2; //apeleytherwnei th lista me ta sums afotou ta steilei sto parent
                struct summary *sum_del_temp;
                for (int iam = 0; iam < num_of_sums; iam++)
                {
                    sum_del_temp = sum_del->next;
                    free(sum_del);
                    sum_del = sum_del_temp;
                }
            }
        }
        else if (read_done == 1) //an einai 1 diabazei thn entolh se chunks
        {
            out_fd = open(argv[2], O_WRONLY);
            while (out_fd < 0)
            {
                out_fd = open(argv[2], O_WRONLY);
            }
            write(out_fd, &ok, sizeof(int)); //apantaei
            close(out_fd);
            int chunks = 0;
            char command[100];

            in_fd = open(argv[1], O_RDONLY);
            while (in_fd < 0)
            {
                in_fd = open(argv[1], O_RDONLY);
            }
            read(in_fd, &chunks, sizeof(int));
            close(in_fd);

            for (int c = 0; c < chunks; c++)
            {
                char *read_chunk = malloc(buffersize + 1);
                out_fd = open(argv[2], O_WRONLY);
                while (out_fd < 0)
                {
                    out_fd = open(argv[2], O_WRONLY);
                }
                write(out_fd, &ok, sizeof(int));
                close(out_fd);

                in_fd = open(argv[1], O_RDONLY);
                while (in_fd < 0)
                {
                    in_fd = open(argv[1], O_RDONLY);
                }
                read(in_fd, read_chunk, buffersize);

                read_chunk[buffersize] = '\0';

                close(in_fd);

                if (c == 0)
                {
                    strcpy(command, read_chunk);
                }
                else
                {

                    strcat(command, read_chunk);
                }
                free(read_chunk);
            }

            char *c1;
            char *c2;
            char *c3;
            char *c4;
            char *c5;

            c1 = strtok(command, "  \n\0");
            if (strcmp(c1, "/searchPatientRecord") == 0)
            {
                c2 = strtok(NULL, "  \n\0");
                int found = 0;
                ////////////////////////////////////

                struct rec *rec_sherlock = records_head;
                while (rec_sherlock != NULL)
                {
                    if (strcmp(rec_sherlock->ID, c2) == 0)
                    {
                        if (rec_sherlock->outDate == 0)
                        {
                            printf("%s %s %s %s %d %ld-%ld-%ld --\n", rec_sherlock->ID, rec_sherlock->first_name, rec_sherlock->last_name, rec_sherlock->disease, rec_sherlock->age, ((rec_sherlock->inDate) % 360) % 30, ((rec_sherlock->inDate) % 360) / 30, (rec_sherlock->inDate) / 360);
                        }
                        else
                        {
                            printf("%s %s %s %s %d %ld-%ld-%ld %ld-%ld-%ld\n", rec_sherlock->ID, rec_sherlock->first_name, rec_sherlock->last_name, rec_sherlock->disease, rec_sherlock->age, ((rec_sherlock->inDate) % 360) % 30, ((rec_sherlock->inDate) % 360) / 30, (rec_sherlock->inDate) / 360, ((rec_sherlock->outDate) % 360) % 30, ((rec_sherlock->outDate) % 360) / 30, (rec_sherlock->outDate) / 360);
                        }
                        found = 1;
                    }
                    rec_sherlock = rec_sherlock->next;
                }
                if (found == 1)
                {
                    req_done++;
                }
                else
                {
                    req_failed++;
                }
                requests++;

                ////////////////////////////////////
                out_fd = open(argv[2], O_WRONLY); //an brike eggrafh thn emfanise kai epestepse 1, alliws epistrefei 0
                while (out_fd < 0)
                {
                    out_fd = open(argv[2], O_WRONLY);
                }
                write(out_fd, &found, sizeof(int));
                close(out_fd);
            }
            else if (strcmp(c1, "/numPatientDischarges") == 0)
            {
                c2 = strtok(NULL, " \0");
                c3 = strtok(NULL, " \0");
                c4 = strtok(NULL, " \0");
                c5 = strtok(NULL, " \n\0");
                if (c5 == NULL)
                {
                    long int from = datetoint(c3);
                    long int to = datetoint(c4);
                    for (int x = 0; x < workload; x++)
                    {
                        int count = 0;
                        // printf("%ld / %ld\n", from, to);
                        struct rec *sherlock = records_head;
                        while (sherlock != NULL)
                        {
                            if (strcmp(sherlock->country, countries_served[x]) == 0 && strcmp(sherlock->disease, c2) == 0 && sherlock->outDate >= from && sherlock->outDate <= to)
                            {
                                count++;
                            }
                            sherlock = sherlock->next;
                        }
                        printf("%s %d\n", countries_served[x], count);
                    }
                }
                else
                {
                    int found = 0;
                    for (int x = 0; x < workload; x++)
                    {
                        if (strcmp(c5, countries_served[x]) == 0)
                        {
                            int count = 0;
                            long int from = datetoint(c3);
                            long int to = datetoint(c4);
                            // printf("%ld / %ld\n", from, to);
                            struct rec *sherlock = records_head;
                            while (sherlock != NULL)
                            {
                                // printf("!!!\n");
                                if (strcmp(sherlock->country, c5) == 0 && strcmp(sherlock->disease, c2) == 0 && sherlock->outDate >= from && sherlock->outDate <= to)
                                {
                                    count++;
                                }
                                sherlock = sherlock->next;
                            }
                            printf("%s %d\n", c5, count);
                            found++;
                        }
                    }
                    if (found == 0)
                    {
                        req_failed++;
                    }
                    else
                    {
                        req_done++;
                    }
                    requests++;
                }

                /////////////////////////////////////////
                out_fd = open(argv[2], O_WRONLY); //apantaei sto parent oti diekperaiwse thn entolh
                while (out_fd < 0)
                {
                    out_fd = open(argv[2], O_WRONLY);
                }
                write(out_fd, &ok, sizeof(int));
                close(out_fd);
            }
        }

        //////////////////////////////////////////////
    }
    char log[30];
    sprintf(log, "./logfiles/logfile.%d", getpid()); //dhmiourgei to logfile
    FILE *lf = fopen(log, "a+");
    for (int i = 0; i < workload; i++)
    {
        fprintf(lf, "%s\n", countries_served[i]);
    }
    fprintf(lf, "TOTAL %d\n", requests);
    fprintf(lf, "SUCCESS %d\n", req_done);
    fprintf(lf, "FAILED %d\n", req_failed);
    fclose(lf);
    // printf("{%d} term\n", getpid());
    for (int i = 0; i < workload; i++)
    {
        struct node *rec_del = head[i];
        struct node *rec_del_temp = NULL;
        while (rec_del != NULL)
        {
            rec_del_temp = rec_del->next;
            free(rec_del);
            rec_del = rec_del_temp;
            // rec_del = rec_sherlock->next;
        }
    }
    struct rec *rec_del = records_head;
    struct rec *rec_del_temp = NULL;
    while (rec_del != NULL)
    {
        // printf("%s | %s | %s | %s | %s | %d\n",rec_del->ID,rec_del->first_name,rec_del->last_name,rec_del->disease,rec_del->country,rec_del->age);
        rec_del_temp = rec_del->next;
        free(rec_del);
        rec_del = rec_del_temp;
        // rec_del = rec_sherlock->next;
    }
    fclose(er);
}

long int datetoint(const char *s) //metatrepei kathe hmeromhnia se enan long int
{
    long int inDate = 0;
    char *temp1 = malloc(strlen(s) + 1);
    strcpy(temp1, s);
    char *temp;

    temp = strtok(temp1, "-");
    inDate = atoi(temp);
    temp = strtok(NULL, "-");
    inDate = inDate + 30 * atoi(temp);
    temp = strtok(NULL, "\0");

    inDate = inDate + 30 * 12 * atoi(temp);

    free(temp1);

    return inDate;
}

void refHandlerChild()
{
    kill(ppid, SIGUSR1);
    refresh = 1;
}
