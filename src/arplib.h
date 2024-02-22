// header file of library "arplib.h"

#ifndef ARPLIB_H
#define ARPLIB_H

// Headers of functions

int spawn(const char *program, char **arg_list);

void writeLog(const char *format, ...);

int sign(int x);

void create_pipe(int pipe_fd[], char string_pipe_fd[][20]);

void recive_correct_pid(int pipe_fd[2], int *pid_address);

void closeAndLog(int fd, const char *descriptorName);

//void error(char *process_name, char *sys_call);



#endif //ARPLIB_H