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
    char str_fd1[2][20];
    
    create_pipe(fd1, str_fd1);

    // Pipe for comommunication between input -> master
    int fd2[2];
    char str_fd2[2][20];
    
    create_pipe(fd2, str_fd2);

    // Pipe for comommunication between drone -> master
    int fd3[2];
    char str_fd3[2][20];
    
    create_pipe(fd3, str_fd3);

    // Pipe for comommunication between target -> master
    int fd4[2];
    char str_fd4[2][20];
    
    create_pipe(fd4, str_fd4);

    // Pipe for communication between obstacle -> master
    int fd5[2];
    char str_fd5[2][20];
    
    create_pipe(fd5, str_fd5);

    // write in log for debug
    writeLog("MASTER send to server --- fd1 file desc: %d, %d ", fd1[0], fd1[1]);
    writeLog("MASTER send to input ---- fd2 file desc: %d, %d ", fd2[0], fd2[1]);
    writeLog("MASTER send to drone ---- fd3 file desc: %d, %d ", fd3[0], fd3[1]);
    writeLog("MASTER send to target --- fd4 file desc: %d, %d ", fd4[0], fd4[1]);
    writeLog("MASTER send to obstacle - fd5 file desc: %d, %d ", fd5[0], fd5[1]);

    //// Pipe for communication between INPUT and SERVER
    int fdi_s[2];
    char str_fdi_s[2][20];
    
    create_pipe(fdi_s, str_fdi_s);

    //// Pipe for communication between DRONE and SERVER
    int fdd_s[2];
    char str_fdd_s[2][20];

    create_pipe(fdd_s, str_fdd_s);

    //// Pipe for communication between SERVER and DRONE
    int fds_d[2];
    char str_fds_d[2][20]; 

    create_pipe(fds_d, str_fds_d);

    //// Pipe for communication between TARGET and SERVER
    int fdt_s[2];
    char str_fdt_s[2][20];

    create_pipe(fdt_s, str_fdt_s);

    //// Pipe for comunication between OBSTACLE and SERVER
    int fdo_s[2];
    char str_fdo_s[2][20];

    create_pipe(fdo_s, str_fdo_s);

    // write log for debug
    writeLog("MASTER to server   fdi_s: %d,%d  fdd_s: %d,%d  fdt_s: %d,%d  fdo_s: %d,%d  fds_d: %d,%d  ", fdi_s[0], fdi_s[1], fdd_s[0], fdd_s[1], fdt_s[0], fdt_s[1], fdo_s[0], fdo_s[1], fds_d[0], fds_d[1]);
    writeLog("MASTER to input    fdi_s: %d,%d  ", fdi_s[0], fdi_s[1]);
    writeLog("MASTER to drone    fdd_s: %d,%d  fds_d: %d,%d  ", fdd_s[0], fdd_s[1], fds_d[0], fds_d[1]);
    writeLog("MASTER to target   fdt_s: %d,%d  ", fdt_s[0], fdt_s[1]);
    writeLog("MASTER to obstacle fdo_s: %d,%d  ", fdo_s[0], fdo_s[1]);

    // --- Rule printing -------------------------------------------------------------------------------------------------
    int fd6[2], rule_pipe[2];
    char str_fd6[2][20], str_rule_pipe[2][20];

    create_pipe(fd6, str_fd6);
    create_pipe(rule_pipe, str_rule_pipe);

    // variabili per recupero informazioni
    int retVal_read;
    char read_buffer[256];

    int rule_pid, real_rule_pid;
    char *arg_list_rule_print[] = {"konsole", "-e", "./rule_print", str_fd6[0], str_fd6[1], str_rule_pipe[0], str_rule_pipe[1],NULL};

    rule_pid = spawn("konsole", arg_list_rule_print);
    writeLog("MASTER spawn rule process with pid: %d ", rule_pid);

    recive_correct_pid(fd6, &real_rule_pid);

    writeLog("MASTER spawn rule process with real pid: %d ", real_rule_pid);

    // recuperare le informazioni per l'apertura del server socket
    if((retVal_read = read(rule_pipe[0], read_buffer, sizeof(read_buffer)))){
        perror("master: read");
        writeLog("==> ERROR ==> master: read, %m ");
    }



    //--- SOCKET SERVER process -------------------------------------------------------------------------------------------------


    // --- GAME SERVER process ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Server process is execute with konsole so, the child_pid(correspond to the pid of the kosole) and the child_pid_received( correspod to the pid of process)
    char *arg_list_server[] = {"konsole", "-e", "./server", str_fd1[0], str_fd1[1], str_fdi_s[0], str_fdi_s[1], str_fdd_s[0], str_fdd_s[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fds_d[0], str_fds_d[1], NULL};
    child_pids[0] = spawn("konsole", arg_list_server);
    writeLog("MASTER spawn server with pid: %d ", child_pids[0]);
    
    recive_correct_pid(fd1, &child_pids_received[0]);

    // --- INPUT process ----------------------------------------------------------------------------------------
    char *arg_list_i[] = {"konsole", "-e", "./input", str_fd2[0], str_fd2[1], str_fdi_s[0], str_fdi_s[1], NULL};
    child_pids[1] = spawn("konsole", arg_list_i);
    writeLog("MASTER spawn input with pid: %d ", child_pids[1]);
    
    recive_correct_pid(fd2, &child_pids_received[1]);

    // --- DRONE process -------------------------------------------------------------------------------------------------
    char *arg_list_drone[] = {"./drone", str_fd3[0], str_fd3[1], str_fdd_s[0], str_fdd_s[1], str_fds_d[0], str_fds_d[1], NULL};
    child_pids[2] = spawn("./drone", arg_list_drone);
    writeLog("MASTER spawn drone with pid: %d ", child_pids[2]);
    
    recive_correct_pid(fd3, &child_pids_received[2]);

    //---- TARGET process -----------------------------------------------------------------------------------------------------
    char *arg_list_target[] = {"./target", str_fd4[0], str_fd4[1], str_fdt_s[0], str_fdt_s[1], NULL};
    child_pids[3] = spawn("./target", arg_list_target);
    writeLog("MASTER spawn target with pid: %d ", child_pids[3]);
    
    recive_correct_pid(fd4, &child_pids_received[3]);

    //---- OBSTACLE process -------------------------------------------------------------------------------------------------------
    char *arg_list_obstacle[] = {"./obstacle", str_fd5[0], str_fd5[1], str_fdo_s[0], str_fdo_s[1], NULL};
    child_pids[4] = spawn("./obstacle", arg_list_obstacle);
    writeLog("MASTER spawn obstacle with pid: %d ", child_pids[4]);
    
    recive_correct_pid(fd5, &child_pids_received[4]);

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