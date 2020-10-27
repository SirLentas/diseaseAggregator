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
#include "../structs/country_list.h"
#include "../structs/summary_list.h"

long int datetoint(const char *s);
struct summary *sums_head = NULL;
int run = 1;
int num_workers = 0;
int *workers_id;

int refresh_time = 0;
void refHandler();
void intHandler();

int main(int argc, char *argv[])
{

    char folder1[20];
    int buffersize;
    int red = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
        {
            strcpy(folder1, argv[i + 1]);
            if (argv[i + 1][strlen(argv[i + 1]) - 1] != '/')
                strcat(folder1, "/");
            // printf("input = %s\n", folder1);
            red++;
        }
        else if (strcmp(argv[i], "-w") == 0)
        {

            num_workers = atoi(argv[i + 1]);
            // printf("num wo=%d\n", num_workers);
            red++;
        }
        else if (strcmp(argv[i], "-b") == 0)
        {

            buffersize = atoi(argv[i + 1]);
            // printf("buffersize = %d\n", buffersize);
            red++;
        }
    }

    if (red != 3)
    {
        printf("./diseaseAggregator –w numWorkers -b bufferSize -i input_dir\n");
        return -1;
    }

    int ok = 1;
    int sums_counter = 0;

    int requests = 0;
    int done = 0;
    int failed = 0;

    DIR *inp_folder = opendir(folder1);
    if (inp_folder == NULL)
    {
        printf("Could not open input directory");
        return 0;
    }

    struct dirent *cntr_folder;
    struct country *cntr_head = NULL;
    int cntr_dirs = 0;
    while ((cntr_folder = readdir(inp_folder)) != NULL) //bazei se mia lista allouw tous ypofakeloys me ta onomata xwrwn
    {
        if (strcmp(cntr_folder->d_name, ".") != 0 && strcmp(cntr_folder->d_name, "..") != 0)
        {
            // printf("%s\n", cntr_folder->d_name);
            cntr_head = c_ins(cntr_head, cntr_folder->d_name);
            cntr_dirs++;
        }
    }

    closedir(inp_folder);

    if (cntr_dirs < num_workers) // an dhmioyrghthikan parapanv worker apo oti zhththikan "apoleiontai oi parapanw"
    {
        printf("%d worker(s) fired\n", num_workers - cntr_dirs);
        num_workers = cntr_dirs;
    }

    char worker_in[num_workers][14];
    char worker_out[num_workers][14];
    int workload[num_workers];
    int temp = 0;
    int num_dirs = cntr_dirs;
    int num_workers_temp = num_workers;
    for (int k = 0; k < num_workers; k++) //gia kathe worker vriskei to workload toy se logikh round robin
    {
        temp = num_dirs / num_workers_temp;
        if ((num_dirs % num_workers_temp) > 0)
        {
            temp++;
        }
        workload[k] = temp;
        num_dirs = num_dirs - temp;
        num_workers_temp--;
    }
    int f_des_in[num_workers];
    int f_des_out[num_workers];

    workers_id = malloc(num_workers * sizeof(int)); //kanei malloc to pinaka poy krataei ta worker ids
    char workers_cntr[num_workers][10];
    pid_t workerpid;

    struct country *cntr_sherlock = cntr_head;

    for (int i = 0; i < num_workers; i++)
    {
        sprintf(worker_in[i], "./worker%d-in", i);
        sprintf(worker_out[i], "./worker%d-out", i);
        if ((workerpid = fork()) != 0)
        {
            remove(worker_in[i]); //diagrafei tyxon pipes poy yparxoyn me to idio onoma
            remove(worker_out[i]);
            mkfifo(worker_in[i], 0666); //dhmiourgei ta nea pipes
            mkfifo(worker_out[i], 0666);
            workers_id[i] = workerpid; //apothikeuei to worker_id
        }
        else
        {
            char wrkld[8];
            sprintf(wrkld, "%d", workload[i]);
            char bf[8];
            sprintf(bf, "%d", buffersize);

            char *arg[6];
            arg[0] = "./worker";
            arg[1] = worker_in[i];
            arg[2] = worker_out[i];
            arg[3] = wrkld;
            arg[4] = malloc(20 * sizeof(char));
            strcpy(arg[4], folder1);
            arg[5] = bf;
            arg[6] = NULL;

            execvp(arg[0], arg);
            // execl("./worker", worker_in[i], worker_out[i], wrkld, folder1, bf, NULL); // start worker and give workload with argument
            exit(-1);
        }
    }

    for (int i = 0; i < num_workers; i++) //gia kathe worker
    {
        for (int j = 0; j < workload[i]; j++) //gia kathe country sto worker
        {
            char *str = malloc(sizeof(char) * strlen(cntr_sherlock->cntr) + 1);
            strcpy(str, cntr_sherlock->cntr);

            int length = strlen(cntr_sherlock->cntr);
            int chunks = length / buffersize; //vriskei se posa chunks ua xvrisei th xwra-mynhma pou prepei na grapseis to pipe
            if ((length % buffersize) > 0)
            {
                chunks++;
            }

            f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
            while (f_des_in[i] < 0)
            {
                f_des_in[i] = open(worker_in[i], O_WRONLY);
            }
            write(f_des_in[i], &chunks, sizeof(int)); //grafei enan int gia na jerei to child posa chunks na diabasei
            close(f_des_in[i]);

            int ok = 0;
            for (int c = 0; c < chunks; c++)
            {
                f_des_out[i] = open(worker_out[i], O_RDONLY); //perimenei na diabasei oti to child diavase to prohgoymeno chunk
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &ok, sizeof(int));
                close(f_des_out[i]);

                char *str2 = malloc(sizeof(char) * buffersize);
                for (int j = 0; j < buffersize; j++)
                {
                    if ((j + c * buffersize) < length)
                        str2[j] = str[j + c * buffersize];
                    else
                    {
                        str2[j] = '\0';
                    }
                }
                f_des_in[i] = open(worker_in[i], O_WRONLY); /////country

                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], str2, buffersize); //grafei to chunk
                close(f_des_in[i]);
                free(str2);
            }

            int ret;
            f_des_out[i] = open(worker_out[i], O_RDONLY);
            while (f_des_out[i] < 0)
            {
                f_des_out[i] = open(worker_out[i], O_RDONLY);
            }
            read(f_des_out[i], &ret, sizeof(int)); //diavazei posa summaries dhmioyrghse to child gia ena arxeio
            close(f_des_out[i]);

            char temp[200];
            for (int s = 0; s < ret; s++) //gia kathe summary
            {
                f_des_in[i] = open(worker_in[i], O_WRONLY);
                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], &ok, sizeof(int)); //eidopoiei mesv pipe ton worker na synexisei
                close(f_des_in[i]);

                int sum_chunks = 0;

                f_des_out[i] = open(worker_out[i], O_RDONLY);
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &sum_chunks, sizeof(int)); //diavazei posa chunks apoteloun ena summary
                close(f_des_out[i]);

                for (int sc = 0; sc < sum_chunks; sc++)
                {
                    char *read_chunk = malloc(buffersize + 1);

                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                    while (f_des_in[i] < 0)
                    {
                        f_des_in[i] = open(worker_in[i], O_WRONLY);
                    }
                    write(f_des_in[i], &ok, sizeof(int)); //eidopoiei mesv pipe ton worker na synexisei me to epomeno chunk
                    close(f_des_in[i]);

                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                    while (f_des_out[i] < 0)
                    {
                        f_des_out[i] = open(worker_out[i], O_RDONLY);
                    }
                    read(f_des_out[i], read_chunk, buffersize); //diavazei to chunk mesw pipe
                    close(f_des_out[i]);

                    read_chunk[buffersize] = '\0';
                    if (sc == 0) //synthetei to summary
                    {
                        strcpy(temp, read_chunk);
                    }
                    else
                    {

                        strcat(temp, read_chunk);
                    }

                    free(read_chunk);
                }
                strcat(temp, "\0");

                char *sum_date;
                char *sum_cntr;
                char *sum_dis;
                int z;
                int t;
                int f;
                int s;

                sum_date = strtok(temp, "/");

                sum_cntr = strtok(NULL, "/");

                sum_dis = strtok(NULL, "/");

                z = atoi(strtok(NULL, "/"));
                t = atoi(strtok(NULL, "/"));
                f = atoi(strtok(NULL, "/"));
                s = atoi(strtok(NULL, "\n\0"));

                sums_head = ins2(sums_head, datetoint(sum_date), sum_cntr, sum_dis, z, t, f, s); //eisagei to summary sth lista toy parent
                sums_counter++;
            }
            free(str);
            cntr_sherlock = cntr_sherlock->next;
        }
    }

    signal(SIGINT, intHandler); //kai stis 2 periptwseis kanei to run=0 kai zhtaei apo ton xrhsth na dwsei mia teleytaia entolh
    signal(SIGQUIT, intHandler);
    signal(SIGUSR1, refHandler);

    int need_worker = 1;
    int no_need_worker = 0;
    int refresh = 2;

    while (run)
    {
        requests++;
        char command[100];
        char command2[100];

        if (refresh_time == 1)
        {
            for (int i = 0; i < num_workers; i++)
            {

                //////////////////////////////////////////
                f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], &refresh, sizeof(int)); //stelnei ton int poy antistoixei se erwtsh gia refresh
                close(f_des_in[i]);

                int answer;

                f_des_out[i] = open(worker_out[i], O_RDONLY);
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &answer, sizeof(int)); //diabazei 1 an ananewthike o worker, 0 an emeine idios
                close(f_des_out[i]);

                if (answer == 1) //an ananewthike o worker
                {

                    f_des_in[i] = open(worker_in[i], O_WRONLY); //grafei ok
                    while (f_des_in[i] < 0)
                    {
                        f_des_in[i] = open(worker_in[i], O_WRONLY);
                    }
                    write(f_des_in[i], &ok, sizeof(int));
                    close(f_des_in[i]);

                    int new_sums;

                    f_des_out[i] = open(worker_out[i], O_RDONLY); //diavazei ton ariyhmo twn sums pou tha labei
                    while (f_des_out[i] < 0)
                    {
                        f_des_out[i] = open(worker_out[i], O_RDONLY);
                    }
                    read(f_des_out[i], &new_sums, sizeof(int));
                    close(f_des_out[i]);

                    printf("[%d] refreshed \n", workers_id[i]);

                    char temp[200];
                    for (int s = 0; s < new_sums; s++) //gia kathe summary
                    {
                        f_des_in[i] = open(worker_in[i], O_WRONLY);
                        while (f_des_in[i] < 0)
                        {
                            f_des_in[i] = open(worker_in[i], O_WRONLY);
                        }
                        write(f_des_in[i], &ok, sizeof(int)); //eidopoiei mesv pipe ton worker na synexisei
                        close(f_des_in[i]);

                        int sum_chunks = 0;

                        f_des_out[i] = open(worker_out[i], O_RDONLY);
                        while (f_des_out[i] < 0)
                        {
                            f_des_out[i] = open(worker_out[i], O_RDONLY);
                        }
                        read(f_des_out[i], &sum_chunks, sizeof(int)); //diavazei posa chunks apoteloun ena summary
                        close(f_des_out[i]);

                        for (int sc = 0; sc < sum_chunks; sc++)
                        {
                            char *read_chunk = malloc(buffersize + 1);

                            f_des_in[i] = open(worker_in[i], O_WRONLY);
                            while (f_des_in[i] < 0)
                            {
                                f_des_in[i] = open(worker_in[i], O_WRONLY);
                            }
                            write(f_des_in[i], &ok, sizeof(int)); //eidopoiei mesv pipe ton worker na synexisei me to epomeno chunk
                            close(f_des_in[i]);

                            f_des_out[i] = open(worker_out[i], O_RDONLY);
                            while (f_des_out[i] < 0)
                            {
                                f_des_out[i] = open(worker_out[i], O_RDONLY);
                            }
                            read(f_des_out[i], read_chunk, buffersize); //diavazei to chunk mesw pipe
                            close(f_des_out[i]);

                            read_chunk[buffersize] = '\0';
                            if (sc == 0) //synthetei to summary
                            {
                                strcpy(temp, read_chunk);
                            }
                            else
                            {

                                strcat(temp, read_chunk);
                            }

                            free(read_chunk);
                        }
                        strcat(temp, "\0");
                        // printf("%s\n",temp);

                        char *sum_date;
                        char *sum_cntr;
                        char *sum_dis;
                        int z;
                        int t;
                        int f;
                        int s;

                        sum_date = strtok(temp, "/");

                        sum_cntr = strtok(NULL, "/");

                        sum_dis = strtok(NULL, "/");

                        z = atoi(strtok(NULL, "/"));
                        t = atoi(strtok(NULL, "/"));
                        f = atoi(strtok(NULL, "/"));
                        s = atoi(strtok(NULL, "\n\0"));

                        sums_head = ins2(sums_head, datetoint(sum_date), sum_cntr, sum_dis, z, t, f, s); //eisagei to summary sth lista toy parent
                        sums_counter++;
                    }
                }
            }
            refresh_time = 0;
        }
        printf("Give command:\n");
        fgets(command, 100, stdin);
        strcpy(command2, command);
        char *c1;
        char *c2;
        char *c3;
        char *c4;
        char *c5;
        char *c6;

        c1 = strtok(command2, "  \n\0");
        if (c1 == NULL)
        {
            requests++;
            failed++;
        }
        else if (strcmp(c1, "/exit") == 0)
        {
            requests--;
            run = 0;
        }
        else if (strcmp(c1, "/listCountries") == 0)
        {
            struct country *cntr_sherlock = cntr_head;
            for (int a = 0; a < num_workers; a++)
            {
                for (int w = 0; w < workload[a]; w++)
                {
                    printf("%s %d\n", cntr_sherlock->cntr, workers_id[a]);
                    cntr_sherlock = cntr_sherlock->next;
                }
            }
            done++;
        }
        else if (strcmp(c1, "/diseaseFrequency") == 0) //metttraei ta peristatika mesa apo ta summaries
        {
            c2 = strtok(NULL, " \0");
            c3 = strtok(NULL, " \0");
            c4 = strtok(NULL, " \0");
            c5 = strtok(NULL, " \n\0");
            if (c5 == NULL)
            {
                long int from = datetoint(c3);
                long int to = datetoint(c4);
                struct country *john = cntr_head;
                int count = 0;
                while (john != NULL)
                {
                    // printf("%ld / %ld\n", from, to);
                    struct summary *sherlock = sums_head;
                    while (sherlock != NULL)
                    {
                        // printf("!!!\n");
                        if (strcmp(sherlock->cntr, john->cntr) == 0 && strcmp(sherlock->dis, c2) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                        {
                            count = count + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                        }
                        sherlock = sherlock->next;
                    }

                    john = john->next;
                }
                printf("%d\n", count);
            }
            else
            {
                int count = 0;
                long int from = datetoint(c3);
                long int to = datetoint(c4);
                // printf("%ld / %ld\n", from, to);
                struct summary *sherlock = sums_head;
                while (sherlock != NULL)
                {
                    // printf("!!!\n");
                    if (strcmp(sherlock->cntr, c5) == 0 && strcmp(sherlock->dis, c2) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                    {
                        count = count + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                    }
                    sherlock = sherlock->next;
                }
                printf("%d\n", count);
            }
            done++;
        }
        else if (strcmp(c1, "/topk-AgeRanges") == 0) //metraei appo ta summaries tis eggrafes kai bgazei ta stats
        {
            c2 = strtok(NULL, " \0");
            c3 = strtok(NULL, " \0");
            c4 = strtok(NULL, " \0");
            c5 = strtok(NULL, " \0");
            c6 = strtok(NULL, " \n\0");
            int k = atoi(c2);
            if (k > 4)
            {
                failed++;
            }
            else
            {

                float total = 0;
                float zero = 0;
                float twenty = 0;
                float forty = 0;
                float sixty = 0;
                long int from = datetoint(c5);
                long int to = datetoint(c6);

                struct summary *sherlock = sums_head;
                while (sherlock != NULL)
                {
                    // printf("!!!\n");
                    if (strcmp(sherlock->cntr, c3) == 0 && strcmp(sherlock->dis, c4) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                    {
                        zero = zero + (sherlock->zero);
                        twenty = twenty + (sherlock->twenty);
                        forty = forty + (sherlock->forty);
                        sixty = sixty + (sherlock->sixty);
                        total = total + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                    }
                    sherlock = sherlock->next;
                }
                for (int t = 0; t < k; t++)
                {
                    float z_per = zero / total;
                    float t_per = twenty / total;
                    float f_per = forty / total;
                    float s_per = sixty / total;

                    if (z_per >= t_per && z_per >= f_per && z_per >= s_per)
                    {
                        printf("0-20: %.1f %%\n", 100 * z_per);
                        zero = -1;
                    }
                    else if (t_per >= z_per && t_per >= f_per && t_per >= s_per)
                    {
                        printf("20-40: %.1f %%\n", 100 * t_per);
                        twenty = -1;
                    }
                    else if (f_per >= t_per && f_per >= z_per && f_per >= s_per)
                    {
                        printf("40-60: %.1f %%\n", 100 * f_per);
                        forty = -1;
                    }
                    else if (s_per >= t_per && s_per >= f_per && s_per >= z_per)
                    {
                        printf("60+: %.1f %%\n", 100 * s_per);
                        sixty = -1;
                    }
                }
                done++;
            }
        }
        else if (strcmp(c1, "/searchPatientRecord") == 0) //workers job
        {
            int length = strlen(command); //proothei thn entolh mesw tou pipe sta child proccesses
            int chunks = length / buffersize;
            if ((length % buffersize) > 0)
            {
                chunks++;
            }
            int total = 0;
            for (int i = 0; i < num_workers; i++)
            {

                //////////////////////////////////////////
                f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], &need_worker, sizeof(int));
                close(f_des_in[i]);

                f_des_out[i] = open(worker_out[i], O_RDONLY);
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &ok, sizeof(int));
                close(f_des_out[i]);

                //////////////////////////////////////////

                f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], &chunks, sizeof(int));
                close(f_des_in[i]);

                int ok = 0;
                for (int c = 0; c < chunks; c++)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                    while (f_des_out[i] < 0)
                    {
                        f_des_out[i] = open(worker_out[i], O_RDONLY);
                    }
                    read(f_des_out[i], &ok, sizeof(int));
                    close(f_des_out[i]);

                    char *str2 = malloc(sizeof(char) * buffersize);
                    for (int j = 0; j < buffersize; j++)
                    {
                        if ((j + c * buffersize) < length)
                            str2[j] = command[j + c * buffersize];
                        else
                        {
                            str2[j] = '\0';
                        }
                    }
                    f_des_in[i] = open(worker_in[i], O_WRONLY); /////country
                    // printf("yuppyyyyyy\n");
                    while (f_des_in[i] < 0)
                    {
                        f_des_in[i] = open(worker_in[i], O_WRONLY);
                    }
                    write(f_des_in[i], str2, buffersize);
                    close(f_des_in[i]);
                    free(str2);
                    // printf("yuppyyyyyy\n");
                }
                int failed_or_not = 0;
                f_des_out[i] = open(worker_out[i], O_RDONLY);
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &failed_or_not, sizeof(int)); //diavazei apo kathe child an vrike kai ektypvse kapoia eggrafh(1 an vrike,0 an oxi)
                close(f_des_out[i]);
                total = total + failed_or_not;
            }
            if (total == 0) //an kanenas child den vrhke emfanizei o parent to no entry found
            {
                printf("No entry found\n");
                failed++;
            }
            else
            {
                done++;
            }
        }
        else if (strcmp(c1, "/numPatientAdmissions") == 0) //anatrexei sta summaries kai metraei tis eisagwges
        {
            c2 = strtok(NULL, " \0");
            c3 = strtok(NULL, " \0");
            c4 = strtok(NULL, " \0");
            c5 = strtok(NULL, " \n\0");
            if (c5 == NULL)
            {
                long int from = datetoint(c3);
                long int to = datetoint(c4);
                struct country *john = cntr_head;
                while (john != NULL)
                {
                    int count = 0;
                    // printf("%ld / %ld\n", from, to);
                    struct summary *sherlock = sums_head;
                    while (sherlock != NULL)
                    {
                        // printf("!!!\n");
                        if (strcmp(sherlock->cntr, john->cntr) == 0 && strcmp(sherlock->dis, c2) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                        {
                            count = count + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                        }
                        sherlock = sherlock->next;
                    }
                    printf("%s %d\n", john->cntr, count);

                    john = john->next;
                }
            }
            else
            {
                int count = 0;
                long int from = datetoint(c3);
                long int to = datetoint(c4);
                // printf("%ld / %ld\n", from, to);
                struct summary *sherlock = sums_head;
                while (sherlock != NULL)
                {
                    // printf("!!!\n");
                    if (strcmp(sherlock->cntr, c5) == 0 && strcmp(sherlock->dis, c2) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                    {
                        count = count + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                    }
                    sherlock = sherlock->next;
                }
                printf("%s %d\n", c5, count);
            }
            done++;
        }
        else if (strcmp(c1, "/numPatientDischarges") == 0) //workers job
        {
            int length = strlen(command); //proothei mesw pipes thn entolh sta child
            int chunks = length / buffersize;
            if ((length % buffersize) > 0)
            {
                chunks++;
            }

            for (int i = 0; i < num_workers; i++)
            {

                //////////////////////////////////////////
                f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], &need_worker, sizeof(int));
                close(f_des_in[i]);

                f_des_out[i] = open(worker_out[i], O_RDONLY);
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &ok, sizeof(int));
                close(f_des_out[i]);

                //////////////////////////////////////////

                f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], &chunks, sizeof(int));
                close(f_des_in[i]);

                int ok = 0;
                for (int c = 0; c < chunks; c++)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                    while (f_des_out[i] < 0)
                    {
                        f_des_out[i] = open(worker_out[i], O_RDONLY);
                    }
                    read(f_des_out[i], &ok, sizeof(int));
                    close(f_des_out[i]);

                    char *str2 = malloc(sizeof(char) * buffersize);
                    for (int j = 0; j < buffersize; j++)
                    {
                        if ((j + c * buffersize) < length)
                            str2[j] = command[j + c * buffersize];
                        else
                        {
                            str2[j] = '\0';
                        }
                    }
                    f_des_in[i] = open(worker_in[i], O_WRONLY); /////country
                    // printf("yuppyyyyyy\n");
                    while (f_des_in[i] < 0)
                    {
                        f_des_in[i] = open(worker_in[i], O_WRONLY);
                    }
                    write(f_des_in[i], str2, buffersize);
                    close(f_des_in[i]);
                    free(str2);
                    // printf("yuppyyyyyy\n");
                }
                f_des_out[i] = open(worker_out[i], O_RDONLY);
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &ok, sizeof(int));
                close(f_des_out[i]);
            }
            done++;
        }
    }

    for (int i = 0; i < num_workers; i++) //eidopoiei ta child oti den xreiazontai pleon mesw pipes stelnontas enan int (no_need_work)
    {
        //////////////////////////////////////////
        f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
        while (f_des_in[i] < 0)
        {
            f_des_in[i] = open(worker_in[i], O_WRONLY);
        }
        write(f_des_in[i], &no_need_worker, sizeof(int));
        close(f_des_in[i]);

        f_des_out[i] = open(worker_out[i], O_RDONLY);
        while (f_des_out[i] < 0)
        {
            f_des_out[i] = open(worker_out[i], O_RDONLY);
        }
        read(f_des_out[i], &ok, sizeof(int));
        close(f_des_out[i]);

        //////////////////////////////////////////
        remove(worker_in[i]);
        remove(worker_out[i]);
    }

    int status; //perimenei ta child na termatisoun
    for (int i = 0; i < num_workers; i++)
    {
        while ((waitpid(workers_id[i], &status, WNOHANG)) == 0)
        {
            sleep(0.5);
        }
    }

    char log[30];
    sprintf(log, "./logfiles/logfile.%d", getpid()); //logfile
    FILE *lf = fopen(log, "a+");

    cntr_sherlock = cntr_head;
    while (cntr_sherlock != NULL)
    {
        fprintf(lf, "%s\n", cntr_sherlock->cntr);
        cntr_sherlock = cntr_sherlock->next;
    }
    fprintf(lf, "TOTAL %d\n", requests);
    fprintf(lf, "SUCCESS %d\n", done);
    fprintf(lf, "FAILED %d\n", failed);
    fclose(lf);

    struct summary *sum_del = sums_head; //apodesmeysh mnhmhw
    struct summary *sum_del_temp;
    for (int iam = 0; iam < sums_counter; iam++)
    {
        // printf("%ld | %s | %s | %d | %d | %d | %d\n",sum_del->int_date,sum_del->cntr,sum_del->dis,sum_del->zero,sum_del->twenty,sum_del->forty,sum_del->sixty);
        sum_del_temp = sum_del->next;
        free(sum_del);
        sum_del = sum_del_temp;
    }

    struct country *c_del = cntr_head;
    struct country *c_del_temp;
    for (int iam = 0; iam < cntr_dirs; iam++)
    {
        c_del_temp = c_del->next;
        free(c_del);
        c_del = c_del_temp;
    }
    free(workers_id);
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

void intHandler()
{
    // red_code = 1;
    printf("\nSIGINT or SIGQUIT\nGive last command:\n");

    run = 0;
}

void refHandler()
{
    refresh_time = 1;
}