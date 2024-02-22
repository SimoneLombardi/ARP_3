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
#include <errno.h>
#include "arplib.h"
#include "../config/config.h"

void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset)
{
    int j = 0;
    for (int i = 0; i < 6; i++)
    {
        fd_array[i][0] = atoi(argv[j + indx_offset]);
        fd_array[i][1] = atoi(argv[j + indx_offset + 1]);
        j += 2;
    }
}


//////////////// controllare tra: widns_size creata con mallok forse non necessario (riga 42), ora nella select ce una nuova variabile, window_size,
// controllre se conviene o meno ettere la alettura della window_size dentro alla select.


int main(int argc, char *argv[])
{

    int i;

    // widnow size
    int *widn_size = malloc(sizeof(int) * 2); // [row, col]

    int fd_array[5][2];
    // str_fd7[0], str_fd7[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fdss_s[0], str_fdss_s[1], str_fds_ss[0], str_fds_ss[1]
    int fd7[2], fdt_s[2], fdo_s[2], fdss_s_t[2], fdss_s_o[2], fds_ss[2];

    printf("argc: %d\n", argc);

    if (argc == 14)
    {
        // gestisci i file descriptor
        pipe_fd_init(fd_array, argv, 2);
    }
    else if (argc == 15)
    {
        // gestisci i file descriptor
        pipe_fd_init(fd_array, argv, 3);
    }

    // visualizza i file descriptor
    for (int i = 0; i < 5; i++)
    {
        printf("fd_array[%d][0] = %d\n", i, fd_array[i][0]);
        printf("fd_array[%d][1] = %d\n", i, fd_array[i][1]);
    }

    // associa i file descriptor alle variabli nominali
    fd7[0] = fd_array[0][0]; // pid pipe
    fd7[1] = fd_array[0][1];

    if (close(fd7[0]) < 0)
    {
        perror("Errore nella chiusura del file descriptor fd7[0]");
        writeLog("===> ERROR ===> socket server: close fd7[0], %m");
    }

    fdt_s[0] = fd_array[1][0]; // target --> socket server
    fdt_s[1] = fd_array[1][1];

    if (close(fdt_s[1]) < 0)
    {
        perror("Errore nella chiusura del file descriptor fdt_s[1]");
        writeLog("===> ERROR ===> socket server: close fdt_s[1], %m");
    }

    fdo_s[0] = fd_array[2][0]; // obstacle --> socket server
    fdo_s[1] = fd_array[2][1];

    if (close(fdo_s[1]) < 0)
    {
        perror("Errore nella chiusura del file descriptor fdo_s[1]");
        writeLog("===> ERROR ===> socket server: close fdo_s[1], %m");
    }

    // da capire cosa bisogna chiudere
    fdss_s_t[0] = fd_array[3][0]; // socket server --> server/ target
    fdss_s_t[1] = fd_array[3][1];

    if (close(fdss_s_t[0]) < 0)
    { // closed reading fd for targhet
        perror("Errore nella chiusura del file descriptor fdss_s_t[0]");
        writeLog("===> ERROR ===> socket server: close fdss_s_t[0], %m");
    }

    fdss_s_o[0] = fd_array[4][0]; // socket server --> server/ obstacle
    fdss_s_o[1] = fd_array[4][1];

    if (close(fdss_s_o[0]) < 0)
    { // closed reading fd for obstacle
        perror("Errore nella chiusura del file descriptor fdss_s_o[0]");
        writeLog("===> ERROR ===> socket server: close fdss_s_o[0], %m");
    }

    fds_ss[0] = fd_array[5][0]; // server --> socket server/window size
    fds_ss[1] = fd_array[5][1];

    writeLog("SOCKET SERVER: fd7      %d, %d", fd7[0], fd7[1]);
    writeLog("SOCKET SERVER: fdt_s    %d, %d", fdt_s[0], fdt_s[1]);
    writeLog("SOCKET SERVER: fdo_s    %d, %d", fdo_s[0], fdo_s[1]);
    writeLog("SOCKET SERVER: fdss_s_t %d, %d", fdss_s_t[0], fdss_s_t[1]);
    writeLog("SOCKET SERVER: fdss_s_o %d, %d", fdss_s_o[0], fdss_s_o[1]);
    writeLog("SOCKET SERVER: fds_ss   %d, %d", fds_ss[0], fds_ss[1]);

    // send pid to server
    int socket_server = getpid();
    if (write(fd7[1], &socket_server, sizeof(pid_t)) < 0)
    {
        perror("write pid in rule_print");
    }
    //////////////////////////////////////////////// inizio lavoro socket server //////////////////////////////////////////////////
    // reciveing the window size
    if (read(fds_ss[0], widn_size, sizeof(int) * 2) < 0)
    {
        perror("read Srow in socket server");
        writeLog("===> ERROR ===> socket server: read Srow, %m");
    }
    writeLog("SOCKET SERVER: window size row: %d, col: %d", widn_size[0], widn_size[1]);

    // variable for select
    int retVal_sel;
    int retVal_read;
    int retVal_write;
    fd_set read_fd;
    struct timeval time_sel;

    // define the array wit all the read file descritor (fdt_s: target -> socket_server, fdo_s: obstacle -> socket_server, fds_ss: server -> socket_server)
    int fd_array[3] = {fdt_s[0], fdo_s[0], fds_ss[0]};
    // find the maximum fd
    int max_fd;
    max_fd = (fdt_s[0] > fdo_s[0]) ? fdt_s[0] : fdo_s[0];
    max_fd = (max_fd > fds_ss[0]) ? max_fd : fds_ss[0];

    // define variable for target, obstacle and window_size
    int window_size[2];
    double set_of_target[MAX_TARG_ARR_SIZE][2];
    double set_of_obstacle[MAX_OBST_ARR_SIZE][2];

    while (1)
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////
        //              PART RELATED TO SELECT FOR CHOSE THE READ FILE DESCRIPTOR                     //
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // define the set of fd
        FD_ZERO(&read_fd);
        FD_SET(fdt_s[0], &read_fd);
        FD_SET(fdo_s[0], &read_fd);
        FD_SET(fds_ss[0], &read_fd);

        // time interval for select
        time_sel.tv_sec = 0;
        time_sel.tv_usec = 3000;
        // do-while statement for avoid problem with signals
        do
        {
            retVal_sel = select(max_fd + 1, &read_fd, NULL, NULL, &time_sel);
        } while (retVal_sel == -1 && errno == EINTR);
        // select for check the value
        if (retVal_sel == -1)
        {
            perror("server: error select: ");
            writeLog("==> ERROR ==> server: select %m ");
            exit(EXIT_FAILURE);
        }
        else if (retVal_sel > 0)
        // case there is some data in the observed file descriptor
        {
            // check wich file descriptor have data inside
            for (i = 0; i < (sizeof(fd_array) / sizeof(int)); i++)
            {
                // check if the fd is inside the ready file descriptor set
                if (FD_ISSET(fd_array[i], &read_fd))
                {

                    if (fd_array[i] == fdt_s[0]) // <<<< target - socket_server >>>>
                    {
                        // read the target from target process
                        do
                        {
                            retVal_read = read(fdt_s[1], set_of_target, sizeof(double) * MAX_TARG_ARR_SIZE * 2);

                        } while (retVal_read == -1 && errno == EINTR);
                        if (retVal_read == -1)
                        // check the error
                        {
                            perror("socket_server: read fdd_s[0]");
                            writeLog("==> ERROR ==> socket_server: read fdt_s[0], %m ");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            // write the target to server
                            do
                            {
                                retVal_write = write(fdt_s[1], set_of_target, sizeof(double) * MAX_TARG_ARR_SIZE * 2);
                            } while (retVal_write == -1 && errno == EINTR);
                            // check for general errors
                            if (retVal_write < 0)
                            {
                                perror("obstacle: error write fdt_s[1]");
                                writeLog("==> ERROR ==> obstacle: write fdt_s[1], %m ");
                                exit(EXIT_FAILURE); // Esce in caso di errore
                            }
                        }
                    }
                    else if (fd_array[i] == fdo_s[0]) // <<<< obstacle - socket_server >>>>
                    {
                        // read the set of obstacle
                        do
                        {
                            retVal_read = read(fdo_s[0], set_of_obstacle, sizeof(double) * MAX_OBST_ARR_SIZE * 2);
                        } while (retVal_read == -1 && errno == EINTR);
                        if (retVal_read == -1)
                        {
                            perror("server: read fdo_s[0]");
                            writeLog("==> ERROR ==> server:read fdo_s[0], %m ");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            // write the obstacle to server
                            do
                            {
                                retVal_write = write(fdo_s[1], set_of_obstacle, sizeof(double) * MAX_OBST_ARR_SIZE * 2);
                            }while(retVal_write == -1 && errno == EINTR);
                            // general write error
                            if ( retVal_write < 0)
                            {
                                perror("obstacle: error write fdo_s[1]");
                                writeLog("==> ERROR ==> obstacle: write fdo_s[1], %m ");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else if (fd_array[i] == fds_ss[0]) // <<<< server - socket_server >>>>
                    {
                        // read the window_size
                        do
                        {
                            retVal_read = read(fds_ss[0], widn_size, sizeof(double) * 2);
                        } while (retVal_read == -1 && errno == EINTR);
                        if (retVal_read == -1)
                        {
                            perror("server: read fdi_s[0]");
                            writeLog("==> ERROR ==> server:read fdi_s[0], %m ");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    return 0;
}