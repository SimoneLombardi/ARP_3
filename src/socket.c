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
int fd7[2], fdt_s[2], fdo_s[2], fdss_s_t[2], fdss_s_o[2], fds_ss[2], fdrp_ss[2];
// server variable
int int_window_size[2]; //[row, col]


void sigusr1Handler(int signum, siginfo_t *info, void *context)
{
    if (signum == SIGUSR1)
    {
        /*send a signal SIGUSR2 to watchdog */
        // printf("SERVER sig handler");
        if (kill(info->si_pid, SIGUSR2) == 0)
        {
            writeLog("SOCKET SERVER: pid %d, received signal from wd pid: %d ", getpid(), info->si_pid);
        }
        else
        {
            error("target: kill SIGUSR2 ");
        }
    }
}


int string_parser(char *string, char *first_arg, char *second_arg);

char parseMessage(const char message[], double array[20][2], int *array_size)
{
    char *msg = (char *)message;
    char id;
    if (sscanf(msg, "%c[%d]", &id, array_size) != 2 || (id != 'O' && id != 'T'))
    {
        writeLog_sock("Failed to parse array size or invalid ID");
        
    }

    msg = strchr(msg, '|');
    if (!msg)
    {
        writeLog_sock("Failed to find start of data");
    }

    for (int i = 0; i < *array_size; i++)
    {
        if (sscanf(msg, "|%lf,%lf", &array[i][0], &array[i][1]) != 2)
        {
            writeLog_sock("Failed to parse pair %d", i);
            return id;
        }

        msg = strchr(msg + 1, '|');
        if (!msg && i != *array_size - 1)
        {
            writeLog_sock("Failed to find separator after pair %d", i);
            return id;
        }
    }
    return id;
}

// function for the communication with the client
void serverHandlingFunction(int newsock_fd, double window_size[])
{
    // buffer for communication
    char buffer_send[MAX_MSG_LENGHT];
    char buffer_rec[MAX_MSG_LENGHT];
    double result_array[20][2]; // array with inside obstacle or target
    char echo[MAX_MSG_LENGHT];
    char client_id[MAX_MSG_LENGHT];
    int n;

    // inizialization communication
    // read client_id (OI or TI)
    bzero(buffer_rec, MAX_MSG_LENGHT);
    n = read(newsock_fd, buffer_rec, sizeof(buffer_rec));
    if (n == -1)
    {
        error("socket_server: read serverHandlingFunction client_ID");
    }
    strcpy(client_id, buffer_rec);
    writeLog_sock("SOCKET SERVER: serverHandlingFunction read client_ID %s", client_id);
   
    // echo client_id
    n = write(newsock_fd, client_id, sizeof(client_id));
    if (n == -1)
    {
        error("socket_server: serverHandlingFunction write (echo) client_ID");
    }
    writeLog_sock("SOCKET SERVER: serverHandlingFunction write echo client_id: %s", client_id);

    // convert double window_size in string
    bzero(buffer_send, MAX_MSG_LENGHT);
    sprintf(buffer_send, "%.3f,%.3f", window_size[0], window_size[1]);
    // write window size to socket
    n = write(newsock_fd, buffer_send, sizeof(buffer_send));
    if (n == -1)
    {
        error("socket_server, serverHandlingFunction write window_size");
    }
    writeLog_sock("SOCKET SERVER: serverHandlingFunction write window_size: %s", buffer_send);
   
    // read the echo of window size
    bzero(echo, MAX_MSG_LENGHT);
    n = read(newsock_fd, echo, sizeof(echo));
    if (n == -1)
    {
        error("socket_server: serverHandlingFunction read echo window_size");
    }
    writeLog_sock("SOCKET SERVER: serverHandlingFunction read echo window_size: %s", echo);
   
    // while loop for the communication of target and obstacle
    while (1)
    {
        // set to zero the buffer
        bzero(buffer_rec, MAX_MSG_LENGHT);

        // read messages comes from client
        n = read(newsock_fd, buffer_rec, sizeof(buffer_rec));
        if (n == -1)
        {
            error("socket_server: serverHandlingFunction read buffer");
        }
        writeLog_sock("SOCKET SERVER: serverHandlingFunction read message: %s", buffer_rec);
       
        // write back the echo to client
        n = write(newsock_fd, buffer_rec, sizeof(buffer_rec));
        if (n == -1)
        {
            error("socket_server: serverHandlingFunction (while(1)) write buffer");
        }
        writeLog_sock("SOCKET SERVER: serverHandlingFunction write echo message: %s", buffer_rec);
        
        int array_size;
        char id;

        // parse the message comes from client, and save the result in result_array
        id = parseMessage(buffer_rec, result_array, &array_size);
        writeLog_sock("--------------------------------------------------------\n");
        writeLog_sock("%c", id);
        // Stampare l'array bidimensionale
        writeLog_sock("Array bidimensionale:");
        for (int i = 0; i < 20; i++)
        {
            writeLog_sock("%lf, %lf\n", result_array[i][0], result_array[i][1]);
        }
        writeLog_sock("--------------------------------------------------------\n");
        // divide the message comes from client for window size
        // now the target and obstacle are normalized
        for (int i = 0; i < array_size; i++)
        {
            result_array[i][0] = result_array[i][0] / window_size[0];
            result_array[i][1] = result_array[i][1] / window_size[1];
        }
        if (id == 'O')
        {
            // server has received an obstacle, need to send to server with pipe fdss_s_o
            n = write(fdss_s_o[1], result_array, sizeof(result_array));
            if (n == -1)
            {
                error("socket_server: serverHandlingFunction write fdss_s_o");
            }
        }
        else if (id == 'T')
        {
            // server has received a target, need to send to game_server with pipe fdss_s_t
            n = write(fdss_s_t[1], result_array, sizeof(result_array));
            if (n == -1)
            {
                error("socket_server: serverHandlingFunction write fdss_s_t");
            }
        }
        else
        {
            writeLog_sock("Invalid ID");
        }
    }
}


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
    while ((ret_n = bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        if (ret_n < 0)
        {
            error("server: binding error");

            port_no++;
            serv_addr.sin_port = htons(port_no);
        }
        else
        {
            writeLog_sock("==> server : binding success\n");
        }
    }

    // set fd to listen
    listen_ret = listen(sock_fd, 5);
    if (listen_ret < 0)
    {
        error("socket_sever_server: fd listening");
    }
    else
    {
        writeLog_sock("== socket_server : fd listening, success\n");
    }

    cli_len = sizeof(cli_addr);

    // reciveing the window size
    if (read(readFD_winSize, int_window_size, sizeof(int) * 2) < 0)
    {
        error("socket server: read fds_ss[0]");
    }

    // cast window size to double
    double window_size[2] = {(double)int_window_size[0], (double)int_window_size[1]};

    writeLog_sock("== server : Local IP: %s\n", inet_ntoa(serv_addr.sin_addr));
    writeLog_sock("== server : Local Port: %d\n", ntohs(serv_addr.sin_port));

    // variabe for fork after acept
    pid_t pid;
    pid_t father_pid = getpid();

    while (1)
    {
        if (getpid() == father_pid)
        {
            // in father pid
            writeLog_sock("server activated\n");

            // accept te connection from client, and generate new file descriptor
            do
            {
                newsock_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_len);
            } while (newsock_fd < 0 && errno == EINTR);
            if (newsock_fd < 0)
            {
                error("socket.server: accept");
            }
            else
            {
                writeLog_sock("Connection established\n");
            }

            if ((pid = fork()) < 0)
            {
                error("socket.server: fork()");
            }
        }
        if (pid == 0)
        {
            // close the socket file descriptor
            // if(close(sock_fd) == -1){
            //    error("socket_server_server, close sock_fd");
            //}
            //  function with oparations socket need to do
            serverHandlingFunction(newsock_fd, window_size);
            exit(EXIT_SUCCESS);
        }
    }
    ////////////////////////
}

void data_conversion(char string_mat[][256], double reading_set[][2], int lenght)
{
    for (int i = 0; i < lenght; i++)
    {
        sprintf(string_mat[i], "%.3f,%.3f", reading_set[i][0], reading_set[i][1]);
        // save positon in a string in the form (y | x)
    }
}

void data_organizer(char string_mat[][256], char send_string[], int lenght, char *client_id)
{
    char header[30];
    bzero(send_string, MAX_MSG_LENGHT);

    sprintf(header, "%c[%d]", client_id[0], lenght);
    // insert the number of obj in the head of the message
    strcat(send_string, header);
    // insert item coords and pipe in the send sendstring
    for (int i = 0; i < lenght; i++)
    {
        strcat(send_string, string_mat[i]);
        if (i < (lenght - 1))
        { // avoid add a pipe after the last element
            strcat(send_string, "|");
        }
    }
}

void client(int port_no_cli, char *string_ip, char *client_ID, int reading_pipe, int lenght)
{
    int n;
    int sock_fd;
    int retR_n, retW_n, ret_n;

    int proc_pid = getpid(); // use the pid to recognise the client process
    char error_msg[100];     // string for error message

    SAI server_address;
    HE *server;

    // comunicatioN variable
    char buffer_send[MAX_MSG_LENGHT]; // for send the data converted in string
    char buffer_rec[MAX_MSG_LENGHT];  // write the received data in string
    char echo[MAX_MSG_LENGHT];
    double window_size[2]; //[row, col]

    ///////////// socket client code /////////////////
    // create the socket of the client
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(error_msg, "ERROR opening socket -- %d", proc_pid);
        error(error_msg);
    }

    // recover server ip address from the string
    if ((server = gethostbyname(string_ip)) == NULL)
    {
        sprintf(error_msg, "ERROR, no such host -- %d", proc_pid);
        error(error_msg);
    }

    // socket initialization
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port_no_cli);

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
            writeLog_sock("SOCKET SERVER: client connected to the server");
        }

    } while (ret_n < 0 && errno == EINTR);

    // communication initiaization

    // send the identifier to server
    n = write(sock_fd, client_ID, sizeof(client_ID));
    if (n == -1)
    {
        error("socket_server: write in socket");
    }
    writeLog_sock("SOCKET SERVER: client write client_ID : %s", client_ID);
    // set to zero the echo array
    bzero(echo, MAX_MSG_LENGHT);

    // read the echo
    n = read(sock_fd, echo, sizeof(echo));
    if (n == -1)
    {
        error("socket_server: write in socket");
    }
    writeLog_sock("SOCKET SERVER: client read echo client_id: %s", echo);
   
    // read the window size from server
    bzero(buffer_rec, MAX_MSG_LENGHT);
    n = read(sock_fd, buffer_rec, sizeof(buffer_rec));
    if (n == -1)
    {
        error("socket_server: read from socket");
    }
    writeLog_sock("SOCKET SERVER: client read window_size %s", buffer_rec);

    // send the window size as echo to server
    n = write(sock_fd, buffer_rec, sizeof(buffer_rec));
    if (n == -1)
    {
        error("socket_server: write in socket");
    }
    // convert dimension window in double
    sscanf(buffer_rec, "%lf,%lf", &window_size[0], &window_size[1]);
   
    // variable for select
    int retVal_sel;
    int retVal_read;
    fd_set read_fd;
    struct timeval time_sel;

    // variable for store the object or target
    double reading_set[MAX_MSG_LENGHT][2];
    char string_mat[MAX_MSG_LENGHT][256];

    // while for send data
    while (1)
    {
        FD_ZERO(&read_fd);
        FD_SET(reading_pipe, &read_fd);

        // time interval for select
        time_sel.tv_sec = 0;
        time_sel.tv_usec = 3000;
        // do-while statement for avoid problem with signals
        do
        {
            // select for avoid
            retVal_sel = select(reading_pipe + 1, &read_fd, NULL, NULL, &time_sel);
        } while (retVal_sel == -1 && errno == EINTR);
        // select for check the value
        if (retVal_sel == -1)
        {
            error("socket_server: select ");
        }
        // if select has data, read from pipe
        else if (retVal_sel > 0)
        {
            do
            {
                retVal_read = read(reading_pipe, reading_set, sizeof(double) * lenght * 2);
            } while (retVal_read == -1 && errno == EINTR);
            // general write error
            if (retVal_read < 0)
            {
                error("socket_server: error read reading_pipe");
            }
            for (int i = 0; i < lenght; i++)
            {
                // multiply the value for the window size
                reading_set[i][0] = reading_set[i][0] * window_size[0];
                reading_set[i][1] = reading_set[i][1] * window_size[1];
            }

            // format the data in string
            data_conversion(string_mat, reading_set, lenght);
            data_organizer(string_mat, buffer_send, lenght, client_ID);
            // write data formatted into socket
            n = write(sock_fd, buffer_send, sizeof(buffer_send));
            if (n == -1)
            {
                error("socket_server: write sock_fd item data");
            }
            writeLog_sock("SOCKET SERVER: client write message: %s", buffer_send);
           
            // set all the buffer field to zero
            bzero(buffer_send, MAX_MSG_LENGHT);

            // read the echo from server
            n = read(sock_fd, buffer_rec, sizeof(buffer_rec));
            if (n == -1)
            {
                error("socket_server: read sock_fd item data");
            }
            writeLog_sock("SOCKET SERVER: client read echo message:%s", buffer_rec);
           
        }
    }

    exit(EXIT_SUCCESS);
}

void pipe_fd_init(int fd_array[][2], char *argv[], int indx_offset);



/////////////////////////////////////////////////////////////////////////////////////////////////
///                       MAIN FUNCTION                                                       ///
/////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // to have all the fd in the two processes
    pid_t socket_server = getpid();
    int i;
    writeLog("SOCKET SERVER created with pid %d ", socket_server);
    writeLog_sock("SOCKET SERVER created with pid %d ", socket_server);

    // configure the handler for sigusr1
    struct sigaction sa_usr1;
    sa_usr1.sa_sigaction = sigusr1Handler;
    sa_usr1.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        error("socket_server: sigaction");
    }

    // widnow size
    int window_size[2]; //[row, col]
    int fd_unpack[7][2];

    pipe_fd_init(fd_unpack, argv, 1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                      MANAGE PIPE                                                                             ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
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
    closeAndLog(fdss_s_t[0], "socket_server: close fdss_s_t[0]");

    // socket server --> server/ obstacle
    fdss_s_o[0] = fd_unpack[4][0];
    fdss_s_o[1] = fd_unpack[4][1];
    closeAndLog(fdss_s_o[0], "socket_server: close fdss_s_o[0]");

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
    ///                      SOCKET PART                                                                          ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Create separate processes for server and client
    pid_t pid = fork();
    if (pid == -1)
    {
        error("socket_server: fork failed");
    }
    else if (pid == 0)
    {
        // Child process
        // prendere ip e address, fare fork e creare i target e obstacle
        char socket_info[100], string_ip[100], string_port_no[100], correct_str_port_no[100];
        int retVall, port_no_cli;
        int obst_pid, targ_pid;
        int readRP_n;

        // recive the socket information from rule print
        if ((readRP_n = read(fdrp_ss[0], socket_info, sizeof(socket_info)) < 0))
        {
            error("soket_server: read fdrp_ss[0]");
        }

        writeLog_sock("socket.main: read socket_info %d , %s", readRP_n, socket_info);
        retVall = string_parser(socket_info, string_ip, string_port_no);

        writeLog_sock("== client : string_ip: %s\n", string_ip);
        writeLog_sock("== client : string_port_no: %s\n", string_port_no);

        port_no_cli = atoi(string_port_no);

        // create a process for obstacle and one for targsocketet
        obst_pid = fork();
        if (obst_pid < 0)
        {
            error("socket.server, client: fork obstacle client");
        }
        if (obst_pid != 0)
        {
            // inside father client process
            targ_pid = fork();
            if (targ_pid < 0)
            {
                error("socket.server, client: fork target client");
            }
        }
        if (obst_pid == 0)
        {
            // inisied client process for obstacle
            client(port_no_cli, string_ip, "OI", fdo_s[0], MAX_OBST_ARR_SIZE);
        }
        if (targ_pid == 0 && obst_pid != 0)
        {
            // inisied client process for targhet
            client(port_no_cli, string_ip, "TI", fdt_s[0], MAX_TARG_ARR_SIZE);
        }
    }
    else
    {
        // Parent process
        server(fds_ss[0]);
    }

    // close all the file descriptor
    closeAndLog(fds_ss[0], "socket_server: close fds_ss[0]");
    closeAndLog(fds_ss[1], "socket_server: close fds_ss[1]");
    closeAndLog(fdrp_ss[0], "socket_server: close fdrp_ss[0]");
    closeAndLog(fdrp_ss[1], "socket_server: close fdrp_ss[1]");
    closeAndLog(fdss_s_o[1], "socket_server: close fdss_s_o[0]");
    closeAndLog(fdss_s_t[1], "socket_server: close fdss_s_t[0]");
    closeAndLog(fdo_s[0], "socket_server: close fdo_s[0]");
    closeAndLog(fdt_s[0], "socket_server: close fdt_s[0]");
    
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