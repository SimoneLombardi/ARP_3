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


int main(int argc, char *argv[]){
    int pid_pipe[2];
    for(int i = 0; i<2; i++){
        pid_pipe[i] = atoi(argv[i+1]);
    }

    int info_pipe[2];
    for(int i = 0; i<2; i++){
        pid_pipe[i] = atoi(argv[i+3]);
    }

    







    return 0;
}