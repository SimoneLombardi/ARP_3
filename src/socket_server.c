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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "arplib.h"
#include "../config/config.h"

#define h_addr h_addr_list[0] /* for backward compatibility */
// the variable h_addr is the first address in the list h_addr_list

void client_handling_function(int pipe_fd, int port_no, char *ip_address);

void server_handling_function(int new_sockfd);

int string_parser(char *string, char *first_arg, char *second_arg);

void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset)
{
    int j = 0;
    for (int i = 0; i < 7; i++)
    {
        fd_array[i][0] = atoi(argv[j + indx_offset]);
        fd_array[i][1] = atoi(argv[j + indx_offset + 1]);
        j += 2;
    }
}

int main(int argc, char *argv[])
{
    int socket_server = getpid();
    int i;

    // widnow size
    int window_size[2]; //[row, col]
    int fd_unpack[7][2];

    // str_fd7[0], str_fd7[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fdss_s[0], str_fdss_s[1], str_fds_ss[0], str_fds_ss[1]
    int fd7[2], fdt_s[2], fdo_s[2], fdss_s_t[2], fdss_s_o[2], fds_ss[2], fdrp_ss[2];

    printf("argc: %d\n", argc);

    pipe_fd_init(fd_unpack, argv, 1);

    // visualizza i file descriptor
    for (int i = 0; i < 7; i++)
    {
        printf("fd_unpack[%d][0] = %d\n", i, fd_unpack[i][0]);
        printf("fd_unpack[%d][1] = %d\n", i, fd_unpack[i][1]);
    }

    // associa i file descriptor alle variabli nominali
    fd7[0] = fd_unpack[0][0]; // pid pipe
    fd7[1] = fd_unpack[0][1];
    // close read file descriptor
    closeAndLog(fd7[0], "socket server: close fd7[0]");
    if (write(fd7[1], &socket_server, sizeof(pid_t)) < 0)
    {
        error("socket_server: write fd7[1]");
    }
    // close write file descriptor
    closeAndLog(fd7[1], "socket_server: close fd7[1]");

    // pipe for communication bentween targer and socket_server
    fdt_s[0] = fd_unpack[1][0];
    fdt_s[1] = fd_unpack[1][1];
    // close the write fd from communication between target and socket_server
    closeAndLog(fdt_s[1], "socket_server: close fdt_s[1]");

    // obstacle --> socket server
    fdo_s[0] = fd_unpack[2][0];
    fdo_s[1] = fd_unpack[2][1];
    // write fd for comm. between obstacle and socket_server
    closeAndLog(fdo_s[1], "socket_server: close fdo_s[1]");

    // socket server --> server/ target/
    fdss_s_t[0] = fd_unpack[3][0];
    fdss_s_t[1] = fd_unpack[3][1];
    closeAndLog(fdss_s_t[0], "socket server: close fdss_s_t[0]");

    // socket server --> server/ obstacle
    fdss_s_o[0] = fd_unpack[4][0];
    fdss_s_o[1] = fd_unpack[4][1];
    closeAndLog(fdss_s_o[0], "socket server: close fdss_s_o[0]");

    // server --> socket server/window size
    fds_ss[0] = fd_unpack[5][0];
    fds_ss[1] = fd_unpack[5][1];

    // socket server --> socket server / socket info
    fdrp_ss[0] = fd_unpack[6][0];
    fdrp_ss[1] = fd_unpack[6][1];

    // print all teh file descriptor received
    writeLog("SOCKET SERVER: fd7      %d, %d", fd7[0], fd7[1]);
    writeLog("SOCKET SERVER: fdt_s    %d, %d", fdt_s[0], fdt_s[1]);
    writeLog("SOCKET SERVER: fdo_s    %d, %d", fdo_s[0], fdo_s[1]);
    writeLog("SOCKET SERVER: fdss_s_t %d, %d", fdss_s_t[0], fdss_s_t[1]);
    writeLog("SOCKET SERVER: fdss_s_o %d, %d", fdss_s_o[0], fdss_s_o[1]);
    writeLog("SOCKET SERVER: fds_ss   %d, %d", fds_ss[0], fds_ss[1]);
    writeLog("SOCKET SERVER: fdrp_ss  %d, %d", fdrp_ss[0], fdrp_ss[1]);

    // socket server activation
    // parent process handles the SERVER side ---> solo se getpid() == father_pid
    // variabili gestione socket
    int sock_fd, newsock_fd, port_no, cli_len, ret_n;
    char string_port_no[100], correct_str_port_no[100];
    char socket_info[100];
    char string_ip[INET_ADDRSTRLEN];

    // socket struct
    SAI serv_addr, cli_addr;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        error("socket_server: error opening socket");
    }
    // set the socket struct to zero
    bzero((char *)&serv_addr, sizeof(serv_addr));

    // set the port number
    port_no = 49152;

    // set the socket struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);

    // bind the tocket
    while ((ret_n = bind(sock_fd, (SA *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        error("socket_server: error on binding");

        port_no = port_no + 1;
        serv_addr.sin_port = htons(port_no);
    }

    inet_ntop(AF_INET, &(serv_addr.sin_addr), string_ip, INET_ADDRSTRLEN);
    printf("Server listening on IP address: %s, Port: %d\n", string_ip, ntohs(serv_addr.sin_port));

    // set fd to listen
    listen(sock_fd, 10);

   
    FILE* fp;
    char buffer[256];
    
    fp = popen("ifconfig | grep -Eo 'inet (addr:)?([0-9]*\\.){3}[0-9]*' | awk '{print $2}'", "r");
    
    if (fp == NULL) {
        perror("Errore nell'apertura del processo");
        return EXIT_FAILURE;
    }

    printf("Gli indirizzi IPv4 trovati sono:\n");

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char ipv4_address[16];
        strncpy(ipv4_address, buffer, 16);
        ipv4_address[16 - 1] = '\0';  // Assicura che la stringa sia terminata correttamente
        printf("%s\n", ipv4_address);
    }

    pclose(fp);

    printf("selected port no: %d", port_no);

    printf("controllo\n");

    // send socket info rule print 
     // reciveing the socket information
    if((read(fdrp_ss[0], socket_info, sizeof(socket_info)) < 0)){
        error("soket server: read fdrp_ss[0]");
    }


    int retVall;
    retVall = string_parser(socket_info, string_ip, string_port_no);

    for (i = 0; i < 5; i++){
        correct_str_port_no[i] = string_port_no[i];
    }

    printf("valore retVall: %d\n", retVall);
    fflush(stdout);

    printf("valori delle stringhe: %s, %s, %s\n", string_ip, string_port_no, socket_info);
    fflush(stdout);

    printf("correct string port no: %s\n", correct_str_port_no);
    fflush(stdout);

    memset(string_port_no, '\0', sizeof(string_port_no));
    strcpy(string_port_no, correct_str_port_no);

    printf("corrected str port no: %s\n", string_port_no);
    fflush(stdout);
    
    

    // reciveing the window size
    if (read(fds_ss[0], window_size, sizeof(int) * 2) < 0)
    {
        error("socket server: read fds_ss[0]");
    }
    writeLog("SOCKET SERVER: window size row: %d, col: %d", window_size[0], window_size[1]);


    if (retVall == 0)
    {
        // SINGLE PLAYER MODE
        printf("singleplayer mode activated\n");
        fflush(stdout);
        
        // variable for select, usefull only for single player
        int retVal_sel;
        int retVal_read;
        int retVal_write;
        fd_set read_fd;
        struct timeval time_sel;

        // define the array wit all the read file descritor (fdt_s: target -> socket_server, fdo_s: obstacle -> socket_server, fds_ss: server -> socket_server)
        int fd_array[2] = {fdt_s[0], fdo_s[0]};
        // find the maximum fd
        int max_fd;
        max_fd = (fdt_s[0] > fdo_s[0]) ? fdt_s[0] : fdo_s[0];
        // max_fd = (max_fd > fds_ss[0]) ? max_fd : fds_ss[0];

        // define variable for target, obstacle and window_size
        double set_of_target[MAX_TARG_ARR_SIZE][2];
        double set_of_obstacle[MAX_OBST_ARR_SIZE][2];

        while (1)
        {
            ////////////////////////////////////////////////////////////////////////////////////////////////
            //              PART RELATED TO SELECT FOR CHOSE THE READ FILE DESCRIPTOR                     //
            ////////////////////////////////////////////////////////////////////////////////////////////////
            // define the set of fd
            FD_ZERO(&read_fd);
            FD_SET(fdt_s[0], &read_fd); // for read target
            FD_SET(fdo_s[0], &read_fd); // for read obstacle
            // FD_SET(fds_ss[0], &read_fd); // for read window size

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
                error("server: error select");
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
                                retVal_read = read(fdt_s[0], set_of_target, sizeof(double) * MAX_TARG_ARR_SIZE * 2);

                            } while (retVal_read == -1 && errno == EINTR);
                            if (retVal_read == -1)
                            // check the error
                            {
                                error("socket_server: read fdd_s[0]");
                            }
                            else
                            {
                                writeLog("/// SOCKET SERVER: controllo byte LETTI target: %d", retVal_read);
                                // write the target to server
                                do
                                {
                                    retVal_write = write(fdss_s_t[1], set_of_target, sizeof(double) * MAX_TARG_ARR_SIZE * 2);
                                } while (retVal_write == -1 && errno == EINTR);
                                // check for general errors
                                if (retVal_write < 0)
                                {
                                    error("obstacle: error write fdt_s[1]");
                                }
                                else
                                {
                                    writeLog("/// SOCKET SERVER: controllo byte SCRITTI target: %d", retVal_write);
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
                                error("server: read fdo_s[0]");
                            }
                            else
                            {
                                writeLog("/// SOCKET SERVER: controllo byte LETTI obstacle: %d", retVal_read);
                                // write the obstacle to server
                                do
                                {
                                    retVal_write = write(fdss_s_o[1], set_of_obstacle, sizeof(double) * MAX_OBST_ARR_SIZE * 2);
                                } while (retVal_write == -1 && errno == EINTR);
                                // general write error
                                if (retVal_write < 0)
                                {
                                    error("socket_server: error write fdo_s[1]");
                                }
                                else
                                {
                                    writeLog("/// SOCKET SERVER: controllo byte SCRITTI obstacle: %d", retVal_write);
                                }
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
    }
    else
    {
        printf("multiplayer mode activated\n");
        fflush(stdout);
        
        // MULTIPLYER MODE
        pid_t targhet_client_pid, obstacle_client_pid;
        pid_t father_pid = getpid();

        if ((targhet_client_pid = fork()) < 0)
        {
            error("socket server: fork targhet_client_pid");
        }

        if (targhet_client_pid != 0)
        {
            if ((obstacle_client_pid = fork()) < 0)
            {
                error("socket server: fork obstacle_client_pid");
            }
        }

        // show the info for connection to screen
        if(getpid() == father_pid){
            
            
            int pid;
            while(1){
                if(getpid() == father_pid){
                    printf("server activated\n");
                    fflush(stdout);

                    newsock_fd = accept(sock_fd, (SA *)&cli_addr, &cli_len);
                    if(newsock_fd < 0){
                        error("socket server: error on accept");
                    }
                    else{
                        printf("Connection established\n");
                        fflush(stdout);
                    }

                    pid = fork();
                    if(pid < 0){
                        error("socket server: error on fork, for client handling");
                    }

                    if(pid == 0){
                        close(sock_fd);
                        server_handling_function(newsock_fd);
                    }
                    else{
                        close(newsock_fd);
                    }
                }
            }
        }

        if (targhet_client_pid == 0)
        {
            // server child process
            sleep(3);
            client_handling_function(fd7[1], atoi(string_port_no), string_ip);
            
        }
        if (obstacle_client_pid == 0)
        {
            // client child process
            sleep(3);
            client_handling_function(fd7[1], atoi(string_port_no), string_ip);
        }

        
    }

    return 0;
}

void client_handling_function(int pipe_fd, int port_no, char *ip_address)
{
    // variabili gestione socket
    printf("attivato client: %d\n", getpid());
    fflush(stdout);

    int sock_fd, port_no_cli;
    int retR_n, retW_n, ret_n;
    int buffer_send;

    // pid of the process
    int proc_dip = getpid(); // use the pid to recognise the client process
    char error_msg[100];     // string for error message

    // socket setruck
    SAI server_addres;
    HE *server;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(error_msg, "ERROR opening socket -- %d", proc_dip);
        error(error_msg);
    }

    if ((server = gethostbyname(ip_address)) == NULL)
    {
        sprintf(error_msg, "ERROR, no such host -- %d", proc_dip);
        error(error_msg);
    } // recover the server ip address

    // socket initialization
    bzero((char *)&server_addres, sizeof(server_addres));
    server_addres.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&server_addres.sin_addr.s_addr, server->h_length);
    // used to copy server->h_lenght byte from string arg1 to string arg2

    server_addres.sin_port = htons(port_no_cli);

    // start communication with the server
    if ((ret_n = connect(sock_fd, (SA *)&server_addres, sizeof(server_addres))) < 0)
    {
        sprintf(error_msg, "ERROR connecting -- %d", proc_dip);
        error(error_msg);
    }
    else
    {
        sprintf(error_msg, "Connected to the server -- %d", proc_dip);
        writeLog(error_msg);
    }

    // communication loop
   /* while (1)
    {
        
        printf("Please enter the message: ");
        bzero(buffer_send, 256);
        fgets(buffer_send, 255, stdin);
        retW_n = write(sock_fd, buffer_send, strlen(buffer_send));
        if (retW_n < 0)
            error("ERROR writing to socket");
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s\n", buffer);
    }*/
    char buffer_send_oo[256];
    char buffer_rec[256];
    int n;

    while(1){
        printf("enter message: ");

        bzero(buffer_send_oo, 256);// set the sending string to zero

        fgets(buffer_send_oo, 255, stdin);// get the message from the user
        
        // check if the user wants to exit
        if(strcmp(buffer_send_oo, "exit") == 0){
            printf("Exiting...\n");
            close(sock_fd);

            exit(0);
        }

        n = write(sock_fd, buffer_send_oo, strlen(buffer_send_oo));
        if (n<0)
            error("ERROR writing to socket, client");

        bzero(buffer_rec, 256);

        n = read(sock_fd, buffer_rec, 255);
        if(n<0)
            error("ERROR reading from socket, client");
        printf("%s\n", buffer_rec);
    }
}

void server_handling_function(int new_sockfd){
    printf("activated server handler -- %d\n", getpid());
    fflush(stdout);
    
    char buffer[256];
    int n;

    // retrive info of the process for client handling
    int pid = getpid();

    bzero(buffer, 256);
    n = read(new_sockfd, buffer, 255);
    if (n < 0)
    {
        error("ERROR reading from socket");
    }

    printf("Here is the message: %s\n", buffer);

    n = write(new_sockfd, "I got your message", 18);
    if (n < 0)
    {
        error("ERROR writing to socket");
    }
}

int string_parser(char *string, char *first_arg, char *second_arg)
{
    // define the char that separate the arguments in the string
    char *separator = " ";
    char *arg;
    int ret_val;
    char temp[256];

    strcpy(temp, string);

    arg = strtok(temp, separator);
    strcpy(first_arg, arg);

    arg = strtok(NULL, separator);
    if (arg == NULL)
    {
        ret_val = 0;
    }
    else
    {
        ret_val = 1;
        strcpy(second_arg, arg);
    }

    return ret_val;
}