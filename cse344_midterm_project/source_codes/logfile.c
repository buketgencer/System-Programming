#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/wait.h> 
#include <dirent.h> 
#include <sys/file.h> 
#include "logfile.h"

 void writeToServerLog(int general_log_fd, char *message) {
        if (write(general_log_fd, message, strlen(message)) == -1) {
            perror("Failed to write to general log file");
            exit(1);
        }
    }


void writeToClientLog(int log_fd, char *message) {
    if (write(log_fd, message, strlen(message)) == -1) {
        perror("Failed to write to client log file");
        exit(1);
    }
}