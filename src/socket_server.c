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
#include <sys/wait.h>
#include <ncurses.h>
#include <ctype.h>
#include "arplib.h"
#include "../config/config.h"

void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset){
    int j = 0;
    for(int i = 0; i < 5; i++){
        fd_array[i][0] = atoi(argv[j + indx_offset]);
        fd_array[i][1] = atoi(argv[j + indx_offset + 1]);

        j += 2;
    }
}

int main(int argc, char *argv[]){

    int fd_array[5][2];
    // str_fd7[0], str_fd7[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fdss_s[0], str_fdss_s[1], str_fds_ss[0], str_fds_ss[1]
    int fd7[2], fdt_s[2], fdo_s[2], fdss_s[2], fds_ss[2];

    if (argc == 12){
        // gestisci i file descriptor
        pipe_fd_init(fd_array, argv, 2);
    }else if(argc == 13){
        // gestisci i file descriptor
        pipe_fd_init(fd_array, argv, 3);
    }

    // visualizza i file descriptor
    for(int i = 0; i < 5; i++){
        printf("fd_array[%d][0] = %d\n", i, fd_array[i][0]);
        printf("fd_array[%d][1] = %d\n", i, fd_array[i][1]);
    }

    // associa i file descriptor alle variabli nominali
    fd7[0] = fd_array[0][0]; // pid pipe
    fd7[1] = fd_array[0][1];

    if(close(fd7[0]) < 0){
        perror("Errore nella chiusura del file descriptor fd7[0]");
    }

    fdt_s[0] = fd_array[1][0]; // target --> socket server
    fdt_s[1] = fd_array[1][1];

    if(close(fdt_s[1]) < 0){
        perror("Errore nella chiusura del file descriptor fdt_s[1]");
    }

    fdo_s[0] = fd_array[2][0]; // obstacle --> socket server
    fdo_s[1] = fd_array[2][1];

    if(close(fdo_s[1]) < 0){
        perror("Errore nella chiusura del file descriptor fdo_s[1]");
    }

    // da capire cosa bisogna chiudere
    fdss_s[0] = fd_array[3][0]; // socket server --> server
    fdss_s[1] = fd_array[3][1];

    fds_ss[0] = fd_array[4][0]; // server --> socket server
    fds_ss[1] = fd_array[4][1];

    writeLog("SOCKET SERVER: fd7 %d, %d", fd7[0], fd7[1]);
    writeLog("SOCKET SERVER: fdt_s %d, %d", fdt_s[0], fdt_s[1]);
    writeLog("SOCKET SERVER: fdo_s %d, %d", fdo_s[0], fdo_s[1]);
    writeLog("SOCKET SERVER: fdss_s %d, %d", fdss_s[0], fdss_s[1]);
    writeLog("SOCKET SERVER: fds_ss %d, %d", fds_ss[0], fds_ss[1]);

    // send pid to server
    int input_pid = getpid();
    if(write(fd7[1], &input_pid, sizeof(input_pid)) < 0){
        perror("write pid in rule_print");
    }

    getchar();
    
    return 0;
}