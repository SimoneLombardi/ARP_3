#include <stdio.h>
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
#include "arplib.h"
#include "../config/config.h"

/*  when signal arrive counter --
    when wd send kill counter ++ */
int counter = 0;
/*signal hadler function*/
void sigusr2Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR2)
    {
        writeLog("WATCHDOG received signal from %d ", info->si_pid);
        counter--;
    }
}

int main(int argc, char *argv[])
{

    pid_t wd_pid = getpid();
    // write into logfile
    writeLog("WATCHDOG is create with pid %d ", wd_pid);
    // In this array I will put all the proces pid converted in int
    int num_ps = PROCESS_NUMBER;
    pid_t pids_from_master[num_ps];
    pid_t pids_from_process[num_ps];
    // declared for all the for cycle
    int i;

    /*configure the handler for sigusr2*/
    struct sigaction sa_usr2;
    sa_usr2.sa_sigaction = sigusr2Handler;
    sa_usr2.sa_flags = SA_SIGINFO; // I need also the info foruse the pid of the process for unde
    if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1)
    {
        error("wd: sigaction");
    }
    // exctract and convert the pid send from master, they are in position 1 => nm_ps +1
    for (i = 1; i < num_ps + 1; i++)
    {
        pids_from_master[i - 1] = atoi(argv[i]);
    }
    // extract and convert in integer the pid received from process position num_ps +1 => 2*num_ps +1
    for (i = num_ps + 1; i < (2 * num_ps) + 1; i++)
    {
        pids_from_process[i - (num_ps + 1)] = atoi(argv[i]);
    }
    // write in logfile all the process received
    for (i = 0; i < num_ps; i++)
    {
        writeLog("WATCHDOG received pid from master: %d", pids_from_master[i]);
    }
    for (i = 0; i < num_ps; i++)
    {
        writeLog("WATCHDOG received pid from process: %d", pids_from_process[i]);
    }
    // infinite loop for the operations of wd
    while (1)
    {
        // Inizialize the counter every time enter in the loop
        counter = 0;
        // send a signal to all process 
        for (i = 0; i < num_ps; i++)
        {
            /* send signal to all process*/
            if (kill(pids_from_process[i], SIGUSR1) != 0)
            {
                error("wd: kill signal SIGUSR1");
            }
            /* increment the counter when send the signal SIGUSR1*/
            counter++;
            sleep(1);
            printf("%d", counter);
            fflush(stdout);
            if (counter == 0)
            {
                // case where the proccess is alive
                writeLog("V - WATCHDOG: Process %d is alive ", pids_from_process[i]);
            }
            else
            {
                // Case where the process do not work
                /*kill all process*/
                for (int j = 0; j < num_ps + 1; j++)
                {
                    if (kill(pids_from_process[j], SIGKILL) == 0)
                    {
                        /*write into logfile that wd close the process*/
                        writeLog(" * WATCHDOG: process %d is closed by wd ", pids_from_process[j]);
                    }
                    else
                    {
                        writeLog("==> ERROR ==> wd: kill signal SIGKILL %m ");
                    }           
                    if (kill(pids_from_master[j], SIGKILL) == 0)
                    {
                        /*write into logfile that wd close the process*/
                        writeLog("WATCHDOG: process %d is closed by wd ", pids_from_master[j]);
                    }
                    else
                    {
                        writeLog("==> ERROR ==> wd: kill signal SIGKILL wd %m ");
                    }
                    /*
                    if (exit(0) == -1){
                        perror("WD: exit() ");
                        writeLog("==> ERROR ==> wd:  ");
     
                    }*/
                    
                }
                return 0;
            }
            sleep(1);
        }

    }
}