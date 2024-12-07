#ifndef __SERVERX_H
#define __SERVERX_H

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


void handle_signal(int signal) {
    if(signal == SIGUSR1 || signal == SIGINT ){
        if(signal == SIGINT){
        if (getpid() == parent_pid) {
        printf(">>Received signal %d, terminating...\n>>bye\n", signal);
        sprintf(server_log_buffer, ">>Received signal %d, terminating...\n>>bye\n", signal);
        write(general_log_fd, server_log_buffer, strlen(server_log_buffer)); 
        }
        }

        // Send kill signal to child processes
        for (int i = 0; i < child_count; i++) {
            kill(child_pids[i], SIGUSR2);
        }
        // send kill signal to clients
        for (int i = 0; i < client_count; i++) {
            // write(client_fds[i], "Server is shutting down.\n", 25);
            kill(client_ids[i], SIGUSR2);
            kill(client_ids[i], SIGKILL);
        }


        // Close and remove client FIFOs
        for (int i = 0; i < child_count; i++) {
            char client_fifo_write[100];
            char client_fifo_read[100];
            sprintf(client_fifo_write, "/tmp/my_named_pipe_write_%d", child_pids[i]);
            sprintf(client_fifo_read, "/tmp/my_named_pipe_read_%d", child_pids[i]);
            close(open(client_fifo_write, O_WRONLY)); // Necessary to break read() in child
            close(open(client_fifo_read, O_RDONLY));  // Necessary to break read() in main
            unlink(client_fifo_write);
            unlink(client_fifo_read);
        }
        // remove main FIFO's
        unlink("/tmp/my_named_pipe_read");
        unlink("/tmp/my_named_pipe_write");

        // release semaphore
        sem_unlink("/my_named_semaphore");
        
        // Close and remove main FIFO
        close(fd_write);
        close(fd_read);

        // Close log file
        if (close(log_fd) == -1) {
            perror("Failed to close log file");
            exit(1);
        }
        if (close(general_log_fd) == -1) {
            perror("Failed to close log file");
            exit(1);
        }


        // Clean up resources
        free(client_ids);
        free(client_fds);
        free(child_pids);

        exit(0);
    }
}


void handle_sigchld(int signal) { // handling when any child process in server terminates
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { // if any of child exit() semaphore will increment
        // Remove pid from child_pids array
        for (int i = 0; i < child_count; i++) { // finds PID and delete it
            if (child_pids[i] == pid) {
                // Shift remaining elements down
                for (int j = i; j < child_count - 1; j++) {
                    child_pids[j] = child_pids[j + 1];
                }
                child_count--;
                break;
            }
        }
    }
    sem_post(semaphore);
}


void handle_client_command(int fd_write, char *command,int clientnum) {

    char *filename;
    char response[512];
    memset(response, 0, sizeof(response));
    if (strncmp(command, "Connect ", 8) == 0) {
        sprintf(response, "Status 1\n");
    }
    else if (strcmp(command, "help") == 0) {
        sprintf(response, "Available commands: help, list, quit, killServer\n");
    } 
    else if (strcmp(command, "list") == 0) {
        DIR *dir;
        struct dirent *entry;
        struct stat statbuf;
        dir = opendir(dirname);  // Open the current directory
        if (dir == NULL) {
            sprintf(response, "Failed to open directory.\n");
        } else {
            strcpy(response, "");  // Clear the response
            char file_path[261]; // Tam yol bilgisini saklamak için bir karakter dizisi
            while ((entry = readdir(dir)) != NULL) {
                sprintf(file_path, "%s/%s", dirname,entry->d_name); // Tam yol bilgisini oluştur
                stat(file_path, &statbuf);
                if (S_ISREG(statbuf.st_mode)) {  // If the entry is a regular file...
                    strcat(response, entry->d_name);
                    strcat(response, "\n");
                }
            }
            closedir(dir);
        }
        write(fd_write, response, strlen(response));
    }
    else if (strncmp(command, "readF ", 6) == 0) {

        char file_buffer[1024];

        int line_number = -1;
        filename = strtok(command + 6, " ");
        char *line_str = strtok(NULL, " ");
        if (line_str != NULL) {
            line_number = atoi(line_str) -1; // for take 1st line as 1
        }
            char path[512];
            sprintf(path, "%s/%s", dirname,filename);


        FILE *file = fopen(path, "r");
        if (file == NULL) {
            sprintf(response, "Failed to open file: %s\n", filename);
            write(fd_write, response, strlen(response));
        } else {
            if(flock(fileno(file), LOCK_EX | LOCK_NB) != 0){
                sprintf(response, "File is locked by another client.\n");
                write(fd_write, response, strlen(response));
            }
            else{
                if (line_number == -1) {
                    // If no line number is given, send the whole file
                    long content_size = 0;
                    fseek(file, 0, SEEK_END);
                    content_size = ftell(file);
                    rewind(file);
                    write(fd_write, &content_size, sizeof(content_size));
                    while (fgets(file_buffer, sizeof(file_buffer), file) != NULL) {
                        write(fd_write, file_buffer, strlen(file_buffer));
                    }
                } else {
                    // If a line number is given, send only that line
                    int current_line = 0;
                    while (fgets(file_buffer, sizeof(file_buffer), file) != NULL) {
                        if (current_line == line_number) {
                            long content_size = strlen(file_buffer);
                            write(fd_write, &content_size, sizeof(content_size));
                            write(fd_write, file_buffer, content_size);
                            break;
                        }
                        current_line++;
                    }
                }
                flock(fileno(file), LOCK_UN);
            }
            fclose(file);
            return;
        }
    }

    else if (strncmp(command, "writeT ", 7) == 0) {
        filename = strtok(command + 7, " ");
        char *line_str = strtok(NULL, " ");
        char *content = strtok(NULL, "");
        if (content == NULL) {
            content = line_str;
            line_str = NULL;
        }
        int line_number = line_str != NULL ? atoi(line_str) : -1;

        char path[512];
        sprintf(path, "%s/%s", dirname,filename);

        FILE *file = fopen(path, "a+");
        if (file == NULL) {
            sprintf(response, "Failed to open or create file: %s\n", filename);
            write(fd_write, response, strlen(response));
        } else {
            if (flock(fileno(file), LOCK_EX | LOCK_NB) != 0) {
                sprintf(response, "File is locked by another client.\n");
                write(fd_write, response, strlen(response));
            } else {
                if (line_number == -1) {
                    // If no line number is given, write to the end of file
                    fputs(content, file);
                    fputs("\n", file);
                } else {
                    char temp_filename[FILENAME_MAX];
                    sprintf(temp_filename, "%s.tmp", filename);
                    FILE *temp_file = fopen(temp_filename, "w");

                    char file_buffer[1024];
                    int current_line = 1;
                    while (fgets(file_buffer, sizeof(file_buffer), file) != NULL) {
                        if (current_line == line_number) {
                            fputs(content, temp_file);
                            fputs("\n", temp_file);
                        } else {
                            fputs(file_buffer, temp_file);
                        }
                        current_line++;
                    }
                    if (current_line <= line_number) {
                        fputs(content, temp_file);
                        fputs("\n", temp_file);
                    }
                    fclose(temp_file);
                    rename(temp_filename, path);
                }
                sprintf(response, "Write successful.\n");
                write(fd_write, response, strlen(response));
                flock(fileno(file), LOCK_UN);
            }
            fclose(file);
            return;
        }
    }

    else if (strncmp(command, "upload ", 7) == 0) {
        // Extract the filename from the command
        filename = strtok(command + 7, " ");
        // Check if the file exists in the client's current directory
        struct stat statbuf;
        if (stat(filename, &statbuf) == -1) {
            sprintf(response, "File does not exist.\n");
        } else if (S_ISDIR(statbuf.st_mode)) {
            sprintf(response, "The name refers to a directory, not a file.\n");
        } else {
            // Check if the file exists in the server's directory
            char file_path[261];
            sprintf(file_path, "%s/%s", dirname, filename);
            if (access(file_path, F_OK) != -1) {
                sprintf(response, "File with the same name already exists in the server's directory.\n");
            } else {
                // Open the file for reading
                FILE *file = fopen(filename, "rb");
                if (file == NULL) {
                    sprintf(response, "Failed to open file.\n");
                } else {
                    // Open the destination file for writing
                    FILE *dest = fopen(file_path, "wb");
                    if (dest == NULL) {
                        sprintf(response, "Failed to create file in the server's directory.\n");
                    } else {
                        // Copy the file in chunks
                        char buffer[1024];
                        size_t total_bytes = 0;
                        size_t bytes;
                        while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {

                            fwrite(buffer, 1, bytes, dest);
                            total_bytes += bytes;
                        }
                        fclose(file);
                        fclose(dest);
                        sprintf(response, "File uploaded successfully. Transferred %zu bytes.\n", total_bytes);
                    }
                }
            }
        }
        write(fd_write, response, strlen(response));
    }


    else if (strncmp(command, "download ", 9) == 0) {
        // Extract the filename from the command
        filename = strtok(command + 9, " ");
        // Check if the file exists in the server's directory
        char file_path[261];
        sprintf(file_path, "%s/%s", dirname, filename);
        struct stat statbuf;
        if (stat(file_path, &statbuf) == -1) {
            sprintf(response, "File does not exist in the server's directory.\n");
        } else if (S_ISDIR(statbuf.st_mode)) {
            sprintf(response, "The name refers to a directory, not a file.\n");
        } else {
            // Check if the file already exists in the client's directory
            if (access(filename, F_OK) != -1) {
                sprintf(response, "File with the same name already exists in the client's directory.\n");
            } else {
                // Copy the file to the client's directory
                FILE *source = fopen(file_path, "rb");
                if (source == NULL) {
                    sprintf(response, "Failed to open file in the server's directory.\n");
                } else {
                    FILE *dest = fopen(filename, "wb");
                    if (dest == NULL) {
                        sprintf(response, "Failed to create file in the client's directory.\n");
                    } else {
                        size_t total_bytes = 0;
                        char buffer[1024];
                        size_t bytes;
                        while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
                            fwrite(buffer, 1, bytes, dest);
                            total_bytes += bytes;
                        }
                        fclose(source);
                        fclose(dest);
                        sprintf(response, "File downloaded successfully. Transferred %zu bytes.\n", total_bytes);
                    }
                }
            }
        }
        write(fd_write, response, strlen(response));
    }
   else if(strncmp(command, "archServer ", 11) == 0){
char *filename = strtok(command + 11, "\n");
    if (filename == NULL) {
        sprintf(response, "No filename provided for archiving.\n");
        write(fd_write, response, strlen(response));
        return;
    }

    printf("Preparing to archive the current contents of the server directory into %s...\n", filename);
    fflush(stdout); // Ensure message is printed before forking

    // Create a temporary directory to store the current contents to be archived
    char tmpDirPath[256];
    snprintf(tmpDirPath, sizeof(tmpDirPath), "%s/tmp_arch_%d", dirname, getpid());
    mkdir(tmpDirPath, 0777);

    // Ensure the temporary directory is empty before copying
    char cleanupCmd[512];
    snprintf(cleanupCmd, sizeof(cleanupCmd), "rm -rf %s/*", tmpDirPath);
    system(cleanupCmd);

    // Copy all files from the server directory to the temporary directory
    char copyCmd[512];
    snprintf(copyCmd, sizeof(copyCmd), "cp -r %s/* %s/", dirname, tmpDirPath);
    system(copyCmd);

    int pid = fork();

    if (pid == 0) { // Child process
        // Change to the temporary directory and archive its contents
        chdir(tmpDirPath);
        char tarCommand[512];
        snprintf(tarCommand, sizeof(tarCommand), "tar -cvf /tmp/%s.tar .", filename);
        execl("/bin/sh", "sh", "-c", tarCommand, (char *) NULL);
        perror("execl failed");
        exit(1);
    } else if (pid > 0) { // Parent process
        int status;
        waitpid(pid, &status, 0); // Wait for the child process to finish

        // Calculate the size of the archived file
        char tarFilePath[256];
        snprintf(tarFilePath, sizeof(tarFilePath), "/tmp/%s.tar", filename);
        struct stat statbuf;
        if (stat(tarFilePath, &statbuf) == -1) {
            sprintf(response, "Failed to archive files or calculate the file size.\n");
        } else {
            // Move the tar file to the original server directory and clean up
            char moveCmd[512];
            snprintf(moveCmd, sizeof(moveCmd), "mv %s %s/", tarFilePath, dirname);
            //print tar file path
            printf("tar file path: %s\n",tarFilePath);
            system(moveCmd);
            snprintf(cleanupCmd, sizeof(cleanupCmd), "rm -rf %s", tmpDirPath);
            system(cleanupCmd);

            sprintf(response, "Successfully archived files in %s. Total size: %ld bytes. Child PID %d\n",
                    filename, statbuf.st_size, pid);
        }
        write(fd_write, response, strlen(response));
    } else {
        sprintf(response, "Failed to fork process for archiving.\n");
        write(fd_write, response, strlen(response));
    }

    //

   
   }
 
    else if (strcmp(command, "quit") == 0) {
        sprintf(response, "client is shutting down.\n");
        write(fd_write, response, strlen(response));

        sprintf(server_log_buffer, "client%d disconnected\n",client_count);
        write(general_log_fd, server_log_buffer, strlen(server_log_buffer));
        printf("client%d disconnected\n",client_count);
        exit(0); // exit child process
    }

    else if (strcmp(command, "killServer") == 0) {
        sprintf(response, "Terminating server.\n");
        write(fd_write, response, strlen(response));

        sprintf(server_log_buffer, ">>Kill signal from client%d.. Terminating...\n>>bye\n",clientnum);
        write(general_log_fd, server_log_buffer, strlen(server_log_buffer));
        printf(">>Kill signal from client%d.. Terminating...\n>>bye\n",clientnum);
        kill(getppid(), SIGUSR1);  // Send SIGUSR1 signal to parent process
    } 
    
    else {
        sprintf(response, "Unknown command: %s\n", command);
        write(fd_write, response, strlen(response));
    }

}

#endif