#ifndef __CLIENTX_H
#define __CLIENTX_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

int fd_write_client;
int fd_read_client;

void handle_signal(int signal) {
    if(signal == SIGUSR2){ //SIGUSR2 value is 12 in linux systems 
        printf("Server is shutting down. Terminating client.\n");
        exit(0);
    }
    else if(signal == SIGINT){
        if (write(fd_write_client, "quit\n", strlen("quit\n")) == -1) { // let server know client exits
                    perror("Failed to send write request to server");
                    exit(1);
        }
        printf("\nBye..\n");

        // Close named pipe and exit
        close(fd_read_client);
        close(fd_write_client);
        exit(0);
    }
    else if(signal == SIGTSTP){
        printf("\nSIGTSTP is not allowed for client!\n If you want to quit , type 'quit'\n");
    }
}


#endif