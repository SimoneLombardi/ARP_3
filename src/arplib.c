/* Library with all the functions of the project*/
#include "arplib.h"
#include <stdio.h>
#include <string.h>
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


void writeLog(const char *format, ...) {
    // Open the log file for appending
    FILE *logfile = fopen("../log/logfile.txt", "a");
    if (logfile == NULL) {
        perror("server: error opening logfile");
        exit(EXIT_FAILURE);
    }

    // Initialize the variable argument list
    va_list args;
    va_start(args, format);

    // Get the current time
    time_t current_time;
    time(&current_time);

    // Get the local time structure
    struct tm *local_time = localtime(&current_time);

    // Get the current time in the format HH:MM:SS
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    // Print the time on a separate line
    fprintf(logfile, "%s =>\t", time_str);

    // Print the log message on a new line with a newline character
    vfprintf(logfile, format, args);
    fprintf(logfile, "\n");

    // Clean up the variable argument list
    va_end(args);

    // Flush the file stream to ensure the message is written immediately
    fflush(logfile);

    // Close the log file, handle errors if closing fails
    if (fclose(logfile) == -1) {
        perror("fclose logfile");
        // Log an error message if closing the file fails
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

// close file descriptor and write on logfile
void closeAndLog(int fd, const char *descriptorName) {
    if (close(fd) < 0) {
        perror(descriptorName);
        // Assuming writeLog is a function for logging errors
        writeLog("ERROR ==> %s, %m", descriptorName);
        exit(EXIT_FAILURE);
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

/*
void error(char *process_name, char *sys_call){
    char temp[100];


    spritf(temp, "%s : %s", process_name, sys_call);
    perror(temp);

    sprintf(temp, "==> ERROR ==> %s: %s", process_name, sys_call);
    strcat(temp, " -- %m");
    writeLog(temp);
}*/
