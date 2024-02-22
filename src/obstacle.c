#include <ncurses.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "arplib.h"
#include "../config/config.h"

//-- Functions header --------------------------------------------
void sigusr1Handler(int signum, siginfo_t *info, void *context);

int main(int argc, char *argv[])
{
    // variable used in for cycle
    int i;
    pid_t obstacle_pid = getpid();
    // write into logfile the pid
    writeLog("OBSTACLE create with pid %d ", obstacle_pid);

    // configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        perror("obstacle: sigaction");
        writeLog("==> ERROR ==> obstacle: sigaction %m ");
        exit(EXIT_FAILURE);
    }

    //// -- manage pipe ----------------------------------------------------------
    // Take the fd for communicating with master; its positions are 1,2 in argv[]
    int fd5[2];
    for (i = 1; i < 3; i++)
    {
        fd5[i - 1] = atoi(argv[i]);
    }
    writeLog("OBSTACLE value of fd5 are: %d %d ", fd5[0], fd5[1]);

    // close the read file descriptor fd1[0]
    if (close(fd5[0]) < 0)
    {
        perror("obstacle: close fd5[1]");
        writeLog("==> ERROR ==> obstacle: close fd5[0], %m ");
        exit(EXIT_FAILURE);
    }
    // write the pid in the pipe
    if (write(fd5[1], &obstacle_pid, sizeof(obstacle_pid)) < 0)
    {
        perror("obstacle: write fd5[1],");
        writeLog("==> ERROR ==> obstacle, write fd5[1] %m ");
        exit(EXIT_FAILURE);
    }
    if (close(fd5[1]) < 0)
    {
        perror("obstacle: close fd5[1]");
        writeLog("==> ERROR ==> obstacle: close fd5[1], %m ");
        exit(EXIT_FAILURE);
    }

    //// pipe for communication between obstacle -> server, are in positions 3, 4 of argv[]
    printf("print in screen the value of argv 3 and 4: %s, %s ", argv[3], argv[4]);

    int fdo_s[2];
    for (i = 3; i < 5; i++)
    {
        fdo_s[i - 3] = atoi(argv[i]);
    }
    writeLog("OBSTACLE value of fdo_s are: %d %d ", fdo_s[0], fdo_s[1]);

    // close the read file descriptor
    if (close(fdo_s[0]) == -1)
    {
        perror("obstacle: close fdo_s[0] ");
        writeLog("==> ERROR ==> obstacle: close fdo_s[0], %m ");
        exit(EXIT_FAILURE);
    }

    // initialize the time on random generator
    time_t t = 0;
    srand(time(NULL));

    double set_of_obstacle[MAX_OBST_ARR_SIZE][2];

    // define write return value
    int retVal_write;

    while (1)
    {
        time_t t = time(NULL);
        for (i = 0; i < MAX_OBST_ARR_SIZE; i++)
        {
            set_of_obstacle[i][0] = (double)rand() / RAND_MAX - 0.5;
            set_of_obstacle[i][1] = (double)rand() / RAND_MAX - 0.5;
        }

        // avoid system call interruption by signal
        do
        {
            retVal_write = write(fdo_s[1], set_of_obstacle, sizeof(double) * MAX_OBST_ARR_SIZE * 2);
        }while(retVal_write == -1 && errno == EINTR);
        // general write error
        if ( retVal_write < 0)
        {
            perror("obstacle: error write fdo_s[1]");
            writeLog("==> ERROR ==> obstacle: write fdo_s[1], %m ");
            exit(EXIT_FAILURE);
        }else{
            writeLog("/// OBSTACLE: controllo byte scritti: %d", retVal_write);
        }
        // generate obstacle every N seconds; implement a non-blocking timer to avoid problems with signals
        time_t t2 = time(NULL);
        while ((t2 - t) < N)
        {
            t2 = time(NULL);
        }
    }

    // close the write file descriptor fdo_s
    if (close(fdo_s[1]) == -1)
    {
        perror("obstacle: close fdo_s[1] ");
        writeLog("==> ERROR ==> obstacle: close fdo_s[1], %m ");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS); // Not reached, but included for completeness
}

//// --- Function section -------------------------------------------------------------

// Insert perror in the kill
void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        /*send a signal SIGUSR2 to watchdog */
        if (kill(info->si_pid, SIGUSR2) == 0)
        {
            writeLog("OBSTACLE: pid %d, received signal from wd pid: %d ", getpid(), info->si_pid);
        }
        else
        {
            perror("obstacle: kill SIGUSR2 ");
            writeLog("==> ERROR ==> obstacle: kill SIGUSR2 %m ");
            exit(EXIT_FAILURE);
        }
    }
}
