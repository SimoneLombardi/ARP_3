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
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "arplib.h"
#include "../config/config.h"

// Functions header
void sigusr1Handler(int signum, siginfo_t *info, void *context);

int main(int argc, char *argv[])
{
    // variable used in for cycle
    int i;
    pid_t target_pid = getpid();
    // write into logfile the pid
    writeLog("TARGET created with pid %d ", target_pid);

    // configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        perror("target: sigaction");
        writeLog("==> ERROR ==> target: sigaction input %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }

    // Take the fd for comunicating with master, it's position is 1,2 in argv[]
    int fd4[2];
    for (i = 1; i < 3; i++)
    {
        fd4[i - 1] = atoi(argv[i]);
    }
    writeLog("TARGET value of fd4 are: %d %d ", fd4[0], fd4[1]);

    // close the read fiel descriptor fd1[0]
    if (close(fd4[0]) < 0)
    {
        perror("target: close fd4[0]");
        writeLog("==> ERROR ==> target: close fd4[0], %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }
    // write the pid in the pipe
    if (write(fd4[1], &target_pid, sizeof(target_pid)) < 0)
    {
        perror("target: write fd4[1],");
        writeLog("==> ERROR ==> target, write fd4[1] %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }
    if (close(fd4[1]) < 0)
    {
        perror("target: close fd4[1]");
        writeLog("==> ERROR ==> target: close fd4[1], %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }

    //// pipe for communication between target->server, are in position 3, 4
    int fdt_s[2];
    for (i = 3; i < 5; i++)
    {
        fdt_s[i - 3] = atoi(argv[i]);
    }
    writeLog("TARGET value of fdt_s are: %d %d ", fdt_s[0], fdt_s[1]);

    // close the read file descriptor, target only write data in the pipe
    if (close(fdt_s[0]) == -1)
    {
        perror("target: close fdt_s[0] ");
        writeLog("==> ERROR ==> target: close fdt_s[0], %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }

    double set_of_target[MAX_TARG_ARR_SIZE][2];
    for (int i = 0; i < MAX_TARG_ARR_SIZE; i++)
    {
        set_of_target[i][0] = ((double)rand() / RAND_MAX) - 0.5;
        set_of_target[i][1] = ((double)rand() / RAND_MAX) - 0.5;
    }

    // define write retVal
    int retVal_write;

    // avoid syscall beign blocked by a signal
    do
    {
        retVal_write = write(fdt_s[1], set_of_target, sizeof(double) * MAX_TARG_ARR_SIZE * 2);
    }while(retVal_write == -1 && errno == EINTR);
    // check for general errors
    if (retVal_write < 0)
    {
        perror("obstacle: error write fdt_s[1]");
        writeLog("==> ERROR ==> obstacle: write fdt_s[1], %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }else{
        writeLog("/// TARGHET: controllo byte scritti: %d", retVal_write);
    }

    while (1)
    {
        sleep(1);
    }

    // close the write file descriptor, target only write data in the pipe
    if (close(fdt_s[1]) == -1)
    {
        perror("target: close fdt_s[1] ");
        writeLog("==> ERROR ==> target: close fdt_s[1], %m ");
        exit(EXIT_FAILURE); // Esce in caso di errore
    }

    return 0; // Aggiunto il ritorno 0 alla fine della funzione main
}

////---- Functions section -----------------------------------------------------------
/* function for write in logfile*/

// Inserire perror nella kill
void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        /*send a signal SIGUSR2 to watchdog */
        // printf("SERVER sig handler");
        if (kill(info->si_pid, SIGUSR2) == 0)
        {
            writeLog("TARGET: pid %d, received signal from wd pid: %d ", getpid(), info->si_pid);
        }
        else
        {
            perror("target: kill SIGUSR2 ");
            writeLog("==> ERROR ==> target: kill SIGUSR2 %m");
            exit(EXIT_FAILURE); // Esce in caso di errore
        }
    }
}
