/* Library with all the functions of the project*/
#include "arplib.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

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

int sign(int x)
{
    if (x < 0)
        return -1;
    else if (x > 0)
        return 1;
    else
        return 1;
}


