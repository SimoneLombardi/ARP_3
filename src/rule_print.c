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
#include <ncurses.h>
#include "arplib.h"
#include "../config/config.h"

void print_screen(char *txt_path, int txt_row, int txt_col, char *buffer);
WINDOW *create_new_window(int row, int col, int ystart, int xstart);
void destroy_win(WINDOW *local_win);


int main(int argc, char *argv[]){

    pid_t input_pid = getpid();
    // write into logfile the pid
    writeLog("RULE_PRINT create with pid %d ", input_pid);

    int pid_pipe[2];
    for(int i = 0; i<2; i++){
        pid_pipe[i] = atoi(argv[i+1]);
    }
    writeLog("RULE PRINT value of pid pipe are: %d, %d ", pid_pipe[0], pid_pipe[1]);

    if(close(pid_pipe[0]) < 0){
        perror("close pid pipe in rule_print");
    }

    if(write(pid_pipe[1], &input_pid, sizeof(input_pid)) < 0){
        perror("write pid in rule_print");
    }

    int info_pipe[2];
    for(int i = 0; i<2; i++){
        info_pipe[i] = atoi(argv[i+3]);
    }
    if(close(info_pipe[0]) < 0){
        perror("close info pipe in rule_print");
    }
    writeLog("RULE PRINT value of INFO pipe are: %d, %d ", info_pipe[0], info_pipe[1]);

    char socket_info[100];

    int Srow, Scol;

    initscr();
    cbreak();
    raw();
    keypad(stdscr, TRUE);

    // definisco i limiti massimi della finsetra, refresh di stdscr
    getmaxyx(stdscr, Srow, Scol);
    refresh();


    // print the rules and wait to start the game
    print_screen("../config/rule.txt", 18, 78, socket_info);

    endwin();

    // send the socket info to the master process
    int Wret;
    if((Wret = write(info_pipe[1], socket_info, strlen(socket_info))) < 0){
        perror("write in rule_print");
        exit(1);
    }

    // close pipe fd
    close(info_pipe[1]);

    return 0;
}

void print_screen(char *txt_path, int txt_row, int txt_col, char *buffer)
{
    // limiti della finestra
    int Srow, Scol;
    getmaxyx(stdscr, Srow, Scol);

    // control variable for the start of the game
    char start_char = '?';
    char resisize_request[] = "please resize the window";
    char rule_line[100];
    char socket_info[100];

    // window pointer
    WINDOW *rule_window;
    WINDOW *socket_info_window;


    // open the graphics file
    FILE *screen_img = fopen(txt_path, "r");
    if (screen_img == NULL)
    {
        printf("null file pointer\n");
        fflush(stdout);
    }

    // check if the window is big enough
    while (Srow < txt_row+4 || Scol < txt_col)
    {
        clear();
        mvaddstr((Srow / 2), ((Scol - strlen(resisize_request)) / 2), resisize_request);
        refresh();

        getmaxyx(stdscr, Srow, Scol);
    }

    //screen reset
    clear();
    refresh();

    //create the window
    rule_window = create_new_window(txt_row+2, Scol, 0, 0);
    socket_info_window = create_new_window(Srow-(txt_row+3), Scol, getmaxy(rule_window), 0);

    // print the rules
    int indx = 1;
    while ((fgets(rule_line, sizeof(rule_line), screen_img)) != NULL)
    {
        mvwprintw(rule_window, indx, (Scol - strlen(rule_line)) / 2, "%s", rule_line);
        indx++;
    }

    // print the socket message info
    sprintf(socket_info, "Insert arguments for socket connection: ");
    mvwprintw(socket_info_window, getmaxy(socket_info_window)/2, 1, "%s", socket_info);
    wmove(socket_info_window, getmaxy(socket_info_window)/2, strlen(socket_info) + 1);
   
    wrefresh(rule_window);
    wrefresh(socket_info_window);

    do{
        wgetnstr(socket_info_window, buffer, sizeof(char)*100);
        
        if(strlen(buffer) == 0){
            wmove(socket_info_window, getmaxy(socket_info_window)/2, strlen(socket_info) + 1);
        }

    }while(strlen(buffer) < 5);

    // remove the new line character
    buffer[strcspn(buffer, "\n")] = 0;

    // close the file
    fclose(screen_img);
}

WINDOW *create_new_window(int row, int col, int ystart, int xstart)
{
    WINDOW *local_window = newwin(row, col, ystart, xstart);
    box(local_window, 0, 0);

    wrefresh(local_window);
    return local_window;
}

void destroy_win(WINDOW *local_win)
{
    box(local_win, ' ', ' ');
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}