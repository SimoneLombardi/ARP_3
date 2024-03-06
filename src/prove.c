#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LENGHT 10
#define MAX_MSG_LENGHT 1024

void data_generation(char string_mat[][256]){
    // random variable
    double yPos, xPos;

    for (int i = 0; i < MAX_LENGHT; i++){
        yPos = ((double)rand() / RAND_MAX);
        xPos = ((double)rand() / RAND_MAX);

        sprintf(string_mat[i], "%.3f,%.3f", yPos, xPos);
        // save positon in a string in the form (y | x)
    }
}

void data_organizer(char string_mat[][256], int rows ,char send_string[MAX_MSG_LENGHT]){
    int obj_num;
    obj_num = rows;
    // define how many obj there are in the array

    char header[30];
    sprintf(header, "O[%d]", obj_num);
    printf("header: %s\n", header);
    printf("number of obj: %d\n", obj_num);
    // insert the number of obj in the head of the message
    strcat(send_string, header);

    for(int i = 0; i < obj_num; i++){
        strcat(send_string, string_mat[i]);
        if(i < (obj_num -1)){
            strcat(send_string, "|");
        }
    }
    printf("\n\n");
}

int main(int argc, char *argv[]){

    char temp_buffer[MAX_LENGHT][256];

    char send_buffer[MAX_MSG_LENGHT];
    memset(send_buffer, 0, sizeof(send_buffer));
    printf("send: %s ... %ld\n", send_buffer, strlen(send_buffer));

    data_generation(temp_buffer);

    for(int i = 0; i < MAX_LENGHT; i++){
        printf("%s\n", temp_buffer[i]);
    }

    data_organizer(temp_buffer, sizeof(temp_buffer)/sizeof(temp_buffer[0]),send_buffer);

    printf("%s\n", send_buffer);

    return 0;
}


