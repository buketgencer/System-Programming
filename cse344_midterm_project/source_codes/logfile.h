#ifndef __LOGFILE_H
#define __LOGFILE_H

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


    


    void writeToServerLog(int general_log_fd, char *message);

    void writeToClientLog(int log_fd, char *message);


#endif