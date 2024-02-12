#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <stdarg.h>
#include "arplib.h"
#include "../config/config.h"


int main()
{
    /* The master spawn all the process  in oreder server, input, drone, target, obstacle, with watchdog at the end
    also create the necessary pipe for the communication between process*/

    // inizialize the variabiles needed
    // all proces without wd
    int num_ps = PROCESS_NUMBER;
    // all process plus watchdog
    int tot_ps = num_ps + 1;
    // array with all the pid of the process execute by master throw execvp
    pid_t child_pids[tot_ps];
    // array with all the pid sended back by process, they are different respect child_pids if the procss was execute throw konsole
    pid_t child_pids_received[num_ps];
    // array with the pid converted in string, the wd is not converted in string
    char str_child_pids[num_ps][20];
    char str_child_pids_received[num_ps][20];
    // inizialize variable for all the forcycle
    int i;

    // Inizialize the log file with mode w, all the data inside will be delete
    FILE *logfile = fopen("../log/logfile.txt", "w");
    if (logfile < 0)
    { // if problem opening file, send error
        perror("master: fopen logfile");
        return 2;
    }
    else
    {
        // wtite in logfile
        time_t current_time;
        // obtain local time
        time(&current_time);
        fprintf(logfile, "=> create MASTER with pid %d: %s ", getpid(), ctime(&current_time));
        if (fclose(logfile) < 0)
        {
            perror("master: fclose logfile");
        }
    }

    // manage pipe------------------------------------------------------------------------
    // Pipe for comommunication between server -> master for send back the pid
    int fd1[2];
    if ((pipe(fd1)) < 0)
    {
        perror("master: pipe fd1");
        writeLog("==> ERROR ==> master: build pipe fd1, %m ");
    }
    // convert fd pipe in str
    char str_fd1[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fd1[i], "%d", fd1[i]);
    }

    // Pipe for comommunication between input -> master
    int fd2[2];
    if ((pipe(fd2)) < 0)
    {
        perror("master: pipe fd2");
        writeLog("==> ERROR ==> master: pipe fd2, %m ");
    }
    // convert fd pipe in str
    char str_fd2[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fd2[i], "%d", fd2[i]);
    }

    // Pipe for comommunication between drone -> master
    int fd3[2];
    if ((pipe(fd3)) < 0)
    {
        perror("master: pipe fd3");
        writeLog("==> ERROR ==> master: pipe fd3, %m ");
    }
    // convert fd pipe in str
    char str_fd3[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fd3[i], "%d", fd3[i]);
    }

    // Pipe for comommunication between target -> master
    int fd4[2];
    if ((pipe(fd4)) < 0)
    {
        perror("master: pipe fd4");
        writeLog("==> ERROR ==> master: pipe fd4, %m ");
    }
    // convert fd pipe in str
    char str_fd4[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fd4[i], "%d", fd4[i]);
    }

    // Pipe for communication between obstacle -> master
    int fd5[2];
    if ((pipe(fd5)) < 0)
    {
        perror("master: pipe fd5");
        writeLog("==> ERROR ==> master: build pipe fd5, %m ");
    }
    // convert fd pipe in str
    char str_fd5[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fd5[i], "%d", fd5[i]);
    }

    // write in log for debug
    writeLog("MASTER send to server the pipe fd1 file descriptor: %d, %d ", fd1[0], fd1[1]);
    writeLog("MASTER send to input the pipe fd2 file descriptor: %d, %d ", fd2[0], fd2[1]);
    writeLog("MASTER send to drone the pipe fd3 file descriptor: %d, %d ", fd3[0], fd3[1]);
    writeLog("MASTER send to target the pipe fd4 file descriptor: %d, %d ", fd4[0], fd4[1]);
    writeLog("MASTER send to obstacle the pipe fd5 file descriptor: %d, %d ", fd5[0], fd5[1]);

    //// Pipe for communication between INPUT and SERVER
    int fdi_s[2];
    if ((pipe(fdi_s)) < 0)
    {
        perror("master: pipe fdi_s ");
        writeLog("==> ERROR ==> master: fdi_s, %m ");
    }
    // Convert fd in sting
    char str_fdi_s[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fdi_s[i], "%d", fdi_s[i]);
    }

    //// Pipe for communication between DRONE and SERVER
    int fdd_s[2];
    if ((pipe(fdd_s)) < 0)
    {
        perror("master: pipe fdd_s ");
        writeLog("==> ERROR ==> master: fdd_s, %m ");
    }
    // Convert fd in sting
    char str_fdd_s[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fdd_s[i], "%d", fdd_s[i]);
    }
    //// Pipe for communication between SERVER and DRONE
    int fds_d[2];
    if ((pipe(fds_d)) < 0)
    {
        perror("master: pipe fds_d ");
        writeLog("==> ERROR ==> master: fds_d, %m ");
    }
    // Convert fd in sting
    char str_fds_d[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fds_d[i], "%d", fds_d[i]);
    }

    //// Pipe for communication between TARGET and SERVER
    int fdt_s[2];
    if ((pipe(fdt_s)) < 0)
    {
        perror("master: pipe fdt_s ");
        writeLog("==> ERROR ==> master: fdt_s, %m ");
    }
    // Convert fd in sting
    char str_fdt_s[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fdt_s[i], "%d", fdt_s[i]);
    }

    //// Pipe for comunication between OBSTACLE and SERVER
    int fdo_s[2];
    if ((pipe(fdo_s)) < 0)
    {
        perror("master: pipe fdo_s ");
        writeLog("==> ERROR ==> master: fdo_s, %m ");
    }
    // Convert fd in sting
    char str_fdo_s[2][20];
    for (i = 0; i < 2; i++)
    {
        sprintf(str_fdo_s[i], "%d", fdo_s[i]);
    }

    // --- SERVER process ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Server process is execute with konsole so, the child_pid(correspond to the pid of the kosole) and the child_pid_received( correspod to the pid of process)
    char *arg_list_server[] = {"konsole", "-e", "./server", str_fd1[0], str_fd1[1], str_fdi_s[0], str_fdi_s[1], str_fdd_s[0], str_fdd_s[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fds_d[0], str_fds_d[1], NULL};
    child_pids[0] = spawn("konsole", arg_list_server);
    writeLog("MASTER spawn server with pid: %d ", child_pids[0]);
    // close the write file descriptor
    if (close(fd1[1]) == -1)
    {
        perror("master: close fd1[1]");
        writeLog("==> ERROR ==> master: fclose fd1[1], %m ");
    }
    // read from pipe, blocking read
    if (read(fd1[0], &child_pids_received[0], sizeof(pid_t)) == -1)
    {
        perror("master: read fd1[0]");
        writeLog("==> ERROR ==> master: read fd1[0], %m ");
    }
    if (close(fd1[0]) == -1)
    {
        perror("master: close fd1[0]");
        writeLog("==> ERROR ==> master: close fd1[0], %m ");
    }

    // --- INPUT process ----------------------------------------------------------------------------------------
    char *arg_list_i[] = {"konsole", "-e", "./input", str_fd2[0], str_fd2[1], str_fdi_s[0], str_fdi_s[1], NULL};
    child_pids[1] = spawn("konsole", arg_list_i);
    writeLog("MSTER spawn input with pid: %d ", child_pids[1]);
    // close the write file descriptor
    if (close(fd2[1]) == -1)
    {
        perror("master: close fd2[1]");
        writeLog("==> ERROR ==> master: fclose fd2[1], %m ");
    }
    // read from pipe, blocking read
    if (read(fd2[0], &child_pids_received[1], sizeof(pid_t)) == -1)
    {
        perror("master: read fd2[0]");
        writeLog("==> ERROR ==> master: read fd2[0], %m ");
    }
    if (close(fd2[0]) == -1)
    {
        perror("master: close fd2[0]");
        writeLog("==> ERROR ==> master: close fd2[0], %m ");
    }
    writeLog("MASTER send to input fdi_s with value: %d, %d ", fdi_s[0], fdi_s[1]);

    // --- DRONE process -------------------------------------------------------------------------------------------------
    char *arg_list_drone[] = {"./drone", str_fd3[0], str_fd3[1], str_fdd_s[0], str_fdd_s[1], str_fds_d[0], str_fds_d[1], NULL};
    child_pids[2] = spawn("./drone", arg_list_drone);
    writeLog("MASTER spawn drone with pid: %d ", child_pids[2]);
    // close the write file descriptor
    if (close(fd3[1]) == -1)
    {
        perror("master: close fd3[1]");
        writeLog("==> ERROR ==> master: close fd3[1], %m ");
    }
    // read from pipe, blocking read
    if (read(fd3[0], &child_pids_received[2], sizeof(pid_t)) == -1)
    {
        perror("master: read fd3[0]");
        writeLog("==> ERROR ==> master: read fd3[0], %m ");
    }
    if (close(fd3[0]) == -1)
    {
        perror("master: close fd3[0]");
        writeLog("==> ERROR ==> master: close fd3[0], %m ");
    }

    //---- TARGET process -----------------------------------------------------------------------------------------------------
    char *arg_list_target[] = {"./target", str_fd4[0], str_fd4[1], str_fdt_s[0], str_fdt_s[1], NULL};
    child_pids[3] = spawn("./target", arg_list_target);
    writeLog("MASTER spawn target with pid: %d ", child_pids[3]);
    // close the write file descriptor
    if (close(fd4[1]) == -1)
    {
        perror("master: close fd4[1]");
        writeLog("==> ERROR ==> master: close fd4[1], %m ");
    }
    // read from pipe, blocking read
    if (read(fd4[0], &child_pids_received[3], sizeof(pid_t)) == -1)
    {
        perror("master: read fd4[0]");
        writeLog("==> ERROR ==> master: read fd4[0], %m ");
    }
    if (close(fd4[0]) == -1)
    {
        perror("master: close fd4[0]");
        writeLog("==> ERROR ==> master: close fd4[0], %m ");
    }

    //---- OBSTACLE process -------------------------------------------------------------------------------------------------------
    char *arg_list_obstacle[] = {"./obstacle", str_fd5[0], str_fd5[1], str_fdo_s[0], str_fdo_s[1], NULL};
    child_pids[4] = spawn("./obstacle", arg_list_obstacle);
    writeLog("MASTER spawn obstacle with pid: %d ", child_pids[4]);
    // close the write file descriptor
    if (close(fd5[1]) == -1)
    {
        perror("master: close fd5[1]");
        writeLog("==> ERROR ==> master: close fd5[1], %m ");
    }
    // read from pipe, blocking read
    if (read(fd5[0], &child_pids_received[4], sizeof(pid_t)) == -1)
    {
        perror("master: read fd5[0]");
        writeLog("==> ERROR ==> master: read fd5[0], %m ");
    }
    if (close(fd5[0]) == -1)
    {
        perror("master: close fd5[0]");
        writeLog("==> ERROR ==> master: close fd5[0], %m ");
    }

    // Convert the array child_pids in string
    for (i = 0; i < num_ps; i++)
    {
        sprintf(str_child_pids[i], "%d", child_pids[i]);
    }
    // Convert the array child_pids_received in string
    for (i = 0; i < num_ps; i++)
    {
        sprintf(str_child_pids_received[i], "%d", child_pids_received[i]);
    }
    writeLog("MASTER: child_pids are: %s, %s, %s, %s, %s ", str_child_pids[0], str_child_pids[1], str_child_pids[2], str_child_pids[3], str_child_pids[4]);
    writeLog("MASTER child_pids_received are: %s, %s, %s, %s, %s", str_child_pids_received[0], str_child_pids_received[1], str_child_pids_received[2], str_child_pids_received[3], str_child_pids_received[4]);
    
    //------- WATCHDOG process --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // spawn watchdog, and pass as argument all the pids of processes
    char *arg_list_wd[] = {"./wd", str_child_pids[0], str_child_pids[1], str_child_pids[2], str_child_pids[3], str_child_pids[4], str_child_pids_received[0], str_child_pids_received[1], str_child_pids_received[2], str_child_pids_received[3], str_child_pids_received[4], NULL};
    child_pids[num_ps] = spawn("./wd", arg_list_wd);
    writeLog("MASTER spawn WATCHDOG with pid: %d ", child_pids[num_ps]);
    // The master will wait until all the process will terminate
    
    pid_t waitResult;
    int status;
    for (i = 0; i <= num_ps; i++)
    {
        waitResult = waitpid(child_pids[i], &status, 0);
        if (waitResult == -1)
        {
            perror("master: waitpid ");
            writeLog("==> ERROR ==> master: waitpid, %m ");
            return 3;
        }
        if (WIFEXITED(status))
        {
            printf("Process %d is termined with status %d\n", i, WEXITSTATUS(status));
            fflush(stdout);
        }
        else
        {
            printf("Process %d is termined with anomaly\n", i);
            fflush(stdout);
        }
    }
    
    return 0;
}
