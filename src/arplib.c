/* Library with all the functions of the project*/
#include "arplib.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>

int spawn(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
        // main process
        return child_pid;
    else
    {
        // child process
        if (execvp(program, arg_list) == -1)
        {
            perror("master: exec ");
            writeLog("==> ERROR ==> exec failed, %m ");
            exit(EXIT_FAILURE);
            return -1;
        }
    }
    return 1;
}

/* function for write in logfile*/
void writeLog(const char *format, ...)
{

    FILE *logfile = fopen("../log/logfile.txt", "a");
    if (logfile == NULL)
    {
        perror("server: error opening logfile");
        exit(EXIT_FAILURE);
    }
    va_list args;
    va_start(args, format);

    time_t current_time;
    time(&current_time);

    fprintf(logfile, "%s => ", ctime(&current_time));
    vfprintf(logfile, format, args);

    va_end(args);
    fflush(logfile);
    if (fclose(logfile) == -1)
    {
        perror("fclose logfile");
        writeLog("ERROR ==> server: fclose logfile");
    }
}

// return sign of arg X
int sign(int x)
{
    if (x < 0)
        return -1;
    else if (x > 0)
        return 1;
    else
        return 1;
}

// create a pipe and convert the value to string for send the fd
void create_pipe(int pipe_fd[], char string_pipe_fd[][20]){
    // create the pipe
    if ((pipe(pipe_fd)) < 0)
    {
        perror("master: pipe fd1");
        writeLog("==> ERROR ==> master: build pipe fd1, %m ");
    }
    // convert fd pipe in str
    for (int i = 0; i < 2; i++)
    {
        sprintf(string_pipe_fd[i], "%d", pipe_fd[i]);
    }
}

// save the real pid of the process
// ARGS: 1) pipe array fd, 2) address of the variable to save the pid ex: &child_pids_received[i]
void recive_correct_pid(int pipe_fd[2], int *pid_address){
    // close the write file descriptor
    if (close(pipe_fd[1]) == -1)
    {   
        perror("master: close RD");
        writeLog("==> ERROR ==> master: close pipe RD, %m ");
    }
    // read from pipe, blocking read
    if (read(pipe_fd[0], pid_address, sizeof(pid_t)) == -1)
    {
        perror("master: read");
        writeLog("==> ERROR ==> master: read, %m ");
    }
    if (close(pipe_fd[0]) == -1)
    {
        perror("master: close WR");
        writeLog("==> ERROR ==> master: close pipe WR, %m ");
    }
}


