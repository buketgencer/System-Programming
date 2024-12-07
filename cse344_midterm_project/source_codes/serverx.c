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
#include <sys/wait.h> // for waitpid
#include <dirent.h> // for perform directory command "list"
#include <sys/file.h> // file locking for avoid issues
#include "logfile.h"


int fd_read; // server main fifo for receive connection requests
int fd_write; // server main fifo for send connection status
int log_fd;
int general_log_fd;

int *client_fds;
int* client_ids;
sem_t *semaphore; // Semaphore for limiting the number of clients
pid_t *child_pids; // array to store child PIDs
int child_count; // count of current child processes
int client_count = 0;
char server_log_buffer[256];
pid_t parent_pid;
char *dirname;

#include "serverx.h"


int main(int argc, char *argv[]) {

    int max_clients;
    parent_pid = getpid();
    struct sigaction sa;

    if (argc != 3) {
        printf("Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
        exit(1);
    }

    dirname = argv[1];
    max_clients = atoi(argv[2]);

    // allocate memorys
    client_ids = (int*)malloc(max_clients * sizeof(int));
    client_fds = (int*)malloc(max_clients * sizeof(int));
    child_pids = (pid_t*)malloc(max_clients * sizeof(pid_t));

    // Create directory if it does not exist
    struct stat st;
    if (stat(dirname, &st) == -1) {
        mkdir(dirname, 0700);
    }

    // Create log file for clients
    char log_file[100];

    // Create log file for all commands
    char log_file_general[100];
    sprintf(log_file_general,"%s/all_logs.txt",dirname);
    general_log_fd = open(log_file_general, O_CREAT | O_WRONLY, 0600); // open general log file
    if (log_fd == -1) {
        perror("Failed to create general log file");
        exit(1);
    }



    sem_unlink("/my_named_semaphore"); // if semaphore already exists, delete it.
    semaphore = sem_open("/my_named_semaphore", O_CREAT | O_EXCL, 0644, max_clients);
    if (semaphore == SEM_FAILED) {
        perror("Failed to create semaphore");
        exit(1);
    }

    
    // Install signal handler for SIGUSR1 and SIGINT
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Failed to install signal handler for SIGUSR1");
        exit(1);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to install signal handler for SIGINT");
        exit(1);
    }



    // Install signal handler for SIGCHLD
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // To automatically restart system calls
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf(">> Server Started PID %d\n", getpid());
    sprintf(server_log_buffer, ">> Server Started PID %d\n", getpid());
    writeToServerLog(general_log_fd,server_log_buffer);


    // Create named pipes for communication with clients

    if (mkfifo("/tmp/my_named_pipe_read", S_IRWXU) < 0) {
        if (errno == EEXIST) {
            unlink("/tmp/my_named_pipe_read");  // remove existing named pipe
            if (mkfifo("/tmp/my_named_pipe_read", S_IRWXU) < 0) {
                perror("mkfifo");
                exit(1);
            }
        } else {
            perror("mkfifo");
            exit(1);
        }
    }

    if (mkfifo("/tmp/my_named_pipe_write", S_IRWXU) < 0) {
        if (errno == EEXIST) {
            unlink("/tmp/my_named_pipe_write");  // remove existing named pipe
            if (mkfifo("/tmp/my_named_pipe_write", S_IRWXU) < 0) {
                perror("mkfifo");
                exit(1);
            }
        } else {
            perror("mkfifo");
            exit(1);
        }
    }

    // Open named pipe for reading
    fd_read = open("/tmp/my_named_pipe_read", O_RDWR);
    if (fd_read == -1) {
        perror("Failed to open named pipe for reading");
        exit(1);
    }

    // Open named pipe for writing
    fd_write = open("/tmp/my_named_pipe_write", O_RDWR);
    if (fd_write == -1) {
        perror("Failed to open named pipe for writing");
        exit(1);
    }

    printf(">> Waiting for clients...\n");
    sprintf(server_log_buffer, ">> Waiting for clients...\n");
    writeToServerLog(general_log_fd,server_log_buffer);




    // Loop to handle client commands
    while (1) {
        // Read client PID from main FIFO
        char buf[100];
        int num_bytes = read(fd_read, buf, sizeof(buf));
        if (num_bytes == -1) {
            perror("Failed to read from named pipe");
            exit(1);
        }

        int client_pid;
        char command[20];
        memset(command, 0, sizeof(command));
        
        sscanf(buf, "%d %s", &client_pid, command); // PID'yi ve komutu oku

        sprintf(log_file, "%s/log_%d.txt", dirname,client_pid); // create clients log file with clients PID
        log_fd = open(log_file, O_CREAT | O_WRONLY, 0600); // opens clients log file 
        if (log_fd == -1) {
            perror("Failed to create log file");
            exit(1);
        }

        
        if (client_pid == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, try again
                continue;
            } else {
                // Error reading from pipe
                perror("Failed to read from named pipe");
                exit(1);
            }
        } else if (client_pid == 0) {
            printf("break out\n");
            // End of file, break out of loop
            break;
        }
        int flag = 0; // for the desired menu design, if the client log in after waits queue, to inform the server 
        if(child_count >=  max_clients){
            if(strcmp("tryConnect",command)  == 0){
            printf(">>Connection request PID %d… Que FULL. Client leaving... \n",client_pid);
            sprintf(server_log_buffer, ">>Connection request PID %d… Que FULL. Client leaving... \n",client_pid);
            writeToServerLog(general_log_fd,server_log_buffer);

                char response[256];
                sprintf(response, "Full");
                write(fd_write, response, strlen(response)); 
                continue;
            }
            else{
                sprintf(server_log_buffer, ">>Connection request PID %d… Que FULL. \n",client_pid);
                writeToServerLog(general_log_fd,server_log_buffer);
                printf(">>Connection request PID %d… Que FULL. \n",client_pid);
            }
            flag = 1;
        }
        else{
            sprintf(server_log_buffer, ">>Client PID:'%d' connected as 'client%d'\n", client_pid,client_count+1);
            writeToServerLog(general_log_fd,server_log_buffer);
            printf(">>Client PID:'%d' connected as 'client%d'\n", client_pid,client_count+1);
        }

        if (sem_wait(semaphore) == -1) {
            perror("Failed to wait on semaphore");
            exit(1);
        }
        if(flag){ // for desired menu design, after queue comes waiting client, server info.
            sprintf(server_log_buffer, ">>Client PID:'%d' connected as 'client%d'\n", client_pid,client_count+1);
            writeToServerLog(general_log_fd,server_log_buffer);
            printf(">>Client PID:'%d' connected as 'client%d'\n", client_pid,client_count+1);
        }
        
        
        client_ids[client_count++] = client_pid; // store client PID's for able to kill all after any killServer command

        // Create new  PID-NAMED FIFOs for communication with client
        char client_fifo_write[100];
        char client_fifo_read[100];
        sprintf(client_fifo_write, "/tmp/my_named_pipe_write_%d", client_pid);
        sprintf(client_fifo_read, "/tmp/my_named_pipe_read_%d", client_pid);

        mkfifo(client_fifo_write, 0666);
        mkfifo(client_fifo_read, 0666);

        // Open client FIFO for writing
        int fd_write_client = open(client_fifo_write, O_RDWR);
        if (fd_write_client == -1) {
            perror("Failed to open client FIFO for writing");
            exit(1);
        }

        // Open client FIFO for reading
        int fd_read_client = open(client_fifo_read, O_RDWR);
        if (fd_read_client == -1) {
            perror("Failed to open client FIFO for reading");
            exit(1);
        }


        // Create new child process to handle client
        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork child process");
            exit(1);
        } else if (pid == 0) {
            int client_numstatic = client_count;
            char response[256];
            sprintf(response, "Status 1");
            write(fd_write, response, strlen(response));  
            // Child process: handle client commands and then terminate
            while(1) {
                int num_bytes = read(fd_read_client, buf, sizeof(buf));
                if (num_bytes <= 0) {
                    break;
                }
                // Remove newline character from end of string
                buf[num_bytes - 1] = '\0';
                

                // Write client command to clients log file
                char client_name[10];
                sprintf(client_name, "client%d", getpid());
                char log_entry[256];
                sprintf(log_entry, "%s\n",buf);
                writeToClientLog(log_fd,log_entry);
                sprintf(log_entry, "%s: %s\n", client_name, buf);
                writeToServerLog(general_log_fd,server_log_buffer);

                handle_client_command(fd_write_client, buf,client_numstatic);
            }
            exit(0);
        } else {
            // Parent process: add child PID to array and continue to next iteration of loop
            if (child_count < max_clients) {
                client_fds[child_count] = fd_write_client;
                child_pids[child_count++] = pid;
            }
            continue;
        }
    }
    return 0;
}