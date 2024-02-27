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

#define h_addr h_addr_list[0]

int string_parser(char *string, char *first_arg, char *second_arg);

void server(int readFD_winSize)
{
    //// server variable ////
    int sock_fd, newsock_fd, port_no, cli_len, ret_n, listen_ret;
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
    port_no = 50000;

    // set the socket struct
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);

    // bind the tocket
    ret_n = bind(sock_fd, (SA *)&serv_addr, sizeof(serv_addr));
    if (ret_n < 0)
    {
        error("server: binding error");
    }
    else
    {
        printf("== server : binding success\n");
    }

    // set fd to listen
    listen_ret = listen(sock_fd, 5);
    if (listen_ret < 0)
    {
        error("sever: fd not listening");
    }
    else
    {
        printf("== server : fd listening, success\n");
    }

    cli_len = sizeof(cli_addr);

    printf("== server : Local IP: %s\n", inet_ntoa(serv_addr.sin_addr));
    printf("== server : Local Port: %d\n", ntohs(serv_addr.sin_port));
    fflush(stdout);

    char buffer[100];
    char read_buffer[100];

    int n;

    while (1)
    {
        printf("server activated\n");
        fflush(stdout);

        newsock_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (newsock_fd < 0)
        {
            error("socket server: error on accept");
            printf("== server : Remote IP: %s\n", inet_ntoa(cli_addr.sin_addr));
            printf("== server : Remote Port: %d\n", ntohs(cli_addr.sin_port));
            fflush(stdout);
        }
        else
        {
            printf("Connection established\n");
            fflush(stdout);
        }

        // read test info
        bzero(buffer, 100);
        printf("send: ");
        fgets(buffer, 100, stdin);

        buffer[strcspn(buffer, "\n")] = 0;

        n = write(newsock_fd, buffer, strlen(buffer));
        if (n < 0)
        {
            error("socket server: error writing to socket");
        }
        
    }
    ////////////////////////
}

void client(int readFD_rule, int readFD_winSize)
{
    // client vairables
    int retVall;
    int window_size[2]; //[row, col]
    char socket_info[100], string_ip[100], string_port_no[100], correct_str_port_no[100];

    int sock_fd, port_no_cli;
    int retR_n, retW_n, ret_n;
    int buffer_send;

    int proc_dip = getpid(); // use the pid to recognise the client process
    char error_msg[100];     // string for error message

    SAI server_address;
    HE *server;

    // recive the socket information from rule print
    if ((read(readFD_rule, socket_info, sizeof(socket_info)) < 0))
    {
        error("soket server: read fdrp_ss[0]");
    }

    retVall = string_parser(socket_info, string_ip, string_port_no);

    printf("== client : string_ip: %s\n", string_ip);
    printf("== client : string_port_no: %s\n", string_port_no);
    fflush(stdout);

    port_no_cli = atoi(string_port_no);
    printf("== client : port_no_cli (after atoi) %d\n", port_no_cli);

    // reciveing the window size
    if (read(readFD_winSize, window_size, sizeof(int) * 2) < 0)
    {
        error("socket server: read fds_ss[0]");
    }
    writeLog("SOCKET SERVER: window size row: %d, col: %d", window_size[0], window_size[1]);

    ///////////// socket client code /////////////////
    // create the socket of the client
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(error_msg, "ERROR opening socket -- %d", proc_dip);
        error(error_msg);
    }

    // recover server ip address from the string
    if ((server = gethostbyname(string_ip)) == NULL)
    {
        sprintf(error_msg, "ERROR, no such host -- %d", proc_dip);
        error(error_msg);
    }

    // socket initialization
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port_no_cli);

    printf("== client : Local IP: %s\n", inet_ntoa(server_address.sin_addr));
    printf("== client : Remote IP: %s\n", inet_ntoa(server_address.sin_addr));

    printf("== client : port_no_cli: %d\n", port_no_cli);
    printf("== client : Remote Port: %d\n", ntohs(server_address.sin_port));

    printf("== client : Host name: %s\n", server->h_name);
    printf("== client : Host address type: %d\n", server->h_addrtype);
    printf("== client : Length: %d\n", server->h_length);
    printf("== client : Host address: %s\n", inet_ntoa(*(struct in_addr *)server->h_addr));
    fflush(stdout);

    // start communication with the server
    do
    {
        if ((ret_n = connect(sock_fd, (SA *)&server_address, sizeof(server_address))) < 0)
        {
            sprintf(error_msg, "client ERROR connecting");
            error(error_msg);
        }
        else
        {
            writeLog("SOCKET SERVER: client connected to the server");
        }
        
    }while(ret_n < 0);

    // send test info to the server
    char buffer[100];

    int n;

    bzero(buffer, 100);
    n = read(sock_fd, buffer, 100);
    if(n < 0){
        error("socket server: error reading from socket");
    }else{
        printf("\nfrom(%s): %s\n", inet_ntoa(*(struct in_addr *)server->h_addr), buffer);
    }


    exit(EXIT_SUCCESS);
}

void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset);

int main(int argc, char *argv[])
{

    ///////////////////////////////////////// set executed by all processes /////////////////////////////////////////
    // to have all the fd in the two processes
    int socket_server = getpid();
    int i;

    // widnow size
    int window_size[2]; //[row, col]
    int fd_unpack[7][2];

    // str_fd7[0], str_fd7[1], str_fdt_s[0], str_fdt_s[1], str_fdo_s[0], str_fdo_s[1], str_fdss_s[0], str_fdss_s[1], str_fds_ss[0], str_fds_ss[1]
    int fd7[2], fdt_s[2], fdo_s[2], fdss_s_t[2], fdss_s_o[2], fds_ss[2], fdrp_ss[2];

    pipe_fd_init(fd_unpack, argv, 1);

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
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Create separate processes for server and client
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Child process
        client(fdrp_ss[0], fds_ss[0]);
    }
    else
    {
        // Parent process
        server(fdrp_ss[0]);
    }
    return 0;
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