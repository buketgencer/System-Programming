#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "clientx.h"



#define min(a,b) ((a) < (b) ? (a) : (b))





int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Invalid usage!\n");
        printf("Usage: %s <Connect/tryConnect> <ServerPID>\n", argv[0]);
        exit(1);
    }


    struct sigaction sa;
    sa.sa_handler = handle_signal; // a function to handle the server shutdown signal
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // Make sure SA_RESTART is not set
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Failed to install signal handler");
        exit(1);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed to install signal handler");
        exit(1);
    }


    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("Sigaction error");
        return 1;
    }

    pid_t server_pid = atoi(argv[2]);
    if (kill(server_pid, 0) == -1) { // for controlling server PID is already exist
        if (errno == ESRCH) {
            printf("Server process does not exist.\n");
            exit(1);
        } else {
            perror("Failed to send signal");
            exit(1);
        }
    }

    // Open named pipe for writing
    int fd_write;
    if (strcmp(argv[1], "Connect") == 0) {
        printf("Waiting for server queue...");
        fflush(stdout); // Flush the buffer to ensure immediate printing waiting.
        while ((fd_write = open("/tmp/my_named_pipe_read", O_WRONLY)) == -1) {
            printf("Failed to connect to server.\n");
            exit(1);
        }

    } else if (strcmp(argv[1], "tryConnect") == 0) {
        fd_write = open("/tmp/my_named_pipe_read", O_WRONLY | O_NONBLOCK);
        if (fd_write == -1) {
            printf("Failed to connect to server.\n");
            exit(1);
        } else {

        }
    } else {
        printf("Invalid connection type. Use 'Connect' or 'tryConnect'.\n");
        exit(1);
    }

    // Open named pipe for reading
    int fd_read = open("/tmp/my_named_pipe_write", O_RDONLY);
    if (fd_read == -1) {
        perror("Failed to open named pipe for reading");
        exit(1);
    }

    // Send connection request to server
    char request[256];
    memset(request, 0, sizeof(request));
    int pid = getpid();
    sprintf(request, "%d %s", pid,argv[1]);
    if (write(fd_write, request, strlen(request)) == -1) {
        perror("Failed to send connection request to server");
        exit(1);
    }



    char response[256];


    memset(response, 0, sizeof(response));

    // receive server connection response
    if (read(fd_read, response, sizeof(response)) == -1) {
        perror("Failed to read response from server");
        exit(1);
    }
    // if client connects with tryConnect, analysis response
    if(strcmp(argv[1],"tryConnect") == 0){
        if(strcmp(response,"Full") == 0){
            printf("Server is Full!\n");
            exit(1);
        }
    }


    // strtok(response, "\n");
    // // Parse response from server
    // char *token = strtok(response, " ");
    // if (strcmp(token, "Status") == 0) {
    //     token = strtok(NULL, " ");
    //     int queue_status = atoi(token);
    //     if (queue_status == 0) {
    //         printf("Server queue is full.\n");
    //     } else {

    //     }
    // } else {
    //     printf("Invalid response from server.\n");
    //     exit(1);
    // }

    // OPEN  CLIENT PID -NAMEDFIFOS 
    char client_fifo_write[100];
    char client_fifo_read[100];
    sprintf(client_fifo_write, "/tmp/my_named_pipe_write_%d", getpid());
    sprintf(client_fifo_read, "/tmp/my_named_pipe_read_%d", getpid());


    // Open client FIFO for writing

    fd_read_client = open(client_fifo_write, O_RDONLY);
    if (fd_write_client == -1) {
        perror("Failed to open client FIFO for writing");
        exit(1);
    }

    // Open client FIFO for reading
    fd_write_client = open(client_fifo_read, O_WRONLY | O_NONBLOCK);
    if (fd_read_client == -1) {
        perror("Failed to open client FIFO for reading");
        exit(1);
    }


    printf("Connection established.\n");
    // Handle commands
    while (1) {
        // Read command from user
        char command[256];

        printf(">>Enter command: ");
        fgets(command, sizeof(command), stdin);
        strtok(command, "\n");

        // Handle commands
        if (strcmp(command, "help") == 0) {
            printf("\t Possible client requests:\n");
            printf("help, list, readF, writeT, upload, download, archServer, quit, killServer\n\n");
        } 
        else if (strcmp(command, "help list") == 0) {
            printf("Usage: list\nDisplay the list of files in Servers directory\n(also displays the list received from the Server)\n");
        }
        else if (strcmp(command, "help readF") == 0) {
            printf("Usage: readF <file> <line #>\nDisplay the # line of the <file>, if no line number is given the whole contents of the file is requested\n");
        }
        else if (strcmp(command, "help writeT") == 0) {
            printf("Usage: writeT <file> <line #> <string>\nWrite the content of “string” to the #th line the <file>\n if the line # is not given writes to the end of file.\nIf the file does not exists in Servers directory\n creates and edits the file at the same time\n");
        }
        else if (strcmp(command, "help upload") == 0) {
            printf("Usage: upload <file>\nUploads the file from the current working directory of client to the Servers directory\n");
        }
        else if (strcmp(command, "help download") == 0) {
            printf("Usage: download <file>\nRequest to receive <file> from Servers directory to client side\n");
        }
        else if(strcmp(command, "help archServer") == 0){
            printf("Usage: archServer <fileName>\nCollect all the files currently available on the the Server side\n and store them in the <filename>.tar archive\n");
        }
        else if(strcmp(command, "help killServer") == 0){
            printf("Usage: killServer\nSends a kill request to the Server\n");
        }
        else if(strcmp(command, "help quit") == 0){
            printf("Usage: quit\nSend write request to Server side log file and quits \n");
        }
        else if(strncmp(command, "archServer ", 11) == 0){
          
            /*
            archServer <fileName>.tar
            Using fork, exec and tar utilities create a child process that will collect all the files currently
            available on the the Server side and store them in the <filename>.tar archive
            */
            /*
            exampla of using tar command from client side:
            >> Enter comment : archServer topuBurda.tar
            Archiving the current contents of the server...
            creating archive directory ...
            5 files downloaded ..125467 bytes transferred ..
            Calling tar utility .. child PID 11967
            child returned with SUCCESS..
            copying the achive file..
            removing achieve directory...
            SUCCESS Server side files are achived in “topuBurada.tar” 
            */

           // implement archServer command here

           char request[512];
              snprintf(request, sizeof(request), "%s\n", command);
                if (write(fd_write_client, request, strlen(request)) == -1) {
                    perror("Failed to send archServer request to server");
                    exit(1);
                }

            // Wait for response from server
            char response[1024];
            memset(response, 0, sizeof(response));
            if (read(fd_read_client, response, sizeof(response)) == -1) {
                perror("Failed to read response from server");
                exit(1);
            }
            printf("Server response:\n%s", response);



        }
        
        else if (strcmp(command, "list") == 0) {
            // Send request to server to list files
            if (write(fd_write_client, "list\n", strlen("list\n")) == -1) {
                perror("Failed to send list request to server");
                exit(1);
            }

            // Wait for response from server
            char file_list[1024];
            memset(file_list, 0, sizeof(file_list));
            if (read(fd_read_client, file_list, sizeof(file_list)) == -1) {
                perror("Failed to read file list from server");
                exit(1);
            }
            printf("Files in server directory:\n%s", file_list);
        } 

        else if (strncmp(command, "readF ", 6) == 0) {
            char request[512];
            snprintf(request, sizeof(request), "%s\n", command);
            if (write(fd_write_client, request, strlen(request)) == -1) {
                perror("Failed to send read request to server");
                exit(1);
            }

            long content_size;
            if (read(fd_read_client, &content_size, sizeof(content_size)) == -1) {
                perror("Failed to read content size from server");
                exit(1);
            }
            int bytes_read;
            long bytes_left = content_size;
            char file_content[1024];
            char buffer[1024];
            if(content_size == 2338328219396892998){ // this value means File is locked by another client.
                bytes_read = read(fd_read_client, buffer, 1024); // for cleaning PIPE buffer for avoid future program logic continue
                continue;
            }
            else if(content_size == 8367798494427701574){ // this value means there is no any file.
                printf("No file found matching this filename\n");
                bytes_read = read(fd_read_client, buffer, 1024); // for cleaning PIPE buffer for avoid future program logic continue
                continue;
            }
            printf("File content:\n");
            while (bytes_left > 0 && (bytes_read = read(fd_read_client, file_content, min(sizeof(file_content) - 1, bytes_left))) > 0) {
                file_content[bytes_read] = '\0';
                printf("%s", file_content);
                bytes_left -= bytes_read;
            }
            if (bytes_read == -1) {
                perror("Failed to read file content from server");
                exit(1);
            }
            printf("\n");
        }



        else if (strncmp(command, "writeT ", 7) == 0) {
            char request[512];
            snprintf(request, sizeof(request), "%s\n", command);
            if (write(fd_write_client, request, strlen(request)) == -1) {
                perror("Failed to send write request to server");
                exit(1);
            }

            // Wait for response from server
            char server_response[512];
            memset(server_response, 0, sizeof(server_response));
            if (read(fd_read_client, server_response, sizeof(server_response)) == -1) {
                perror("Failed to read server response");
                exit(1);
            }
            printf("%s", server_response);
        }



        else if (strncmp(command, "upload ", 7) == 0) {
            char request[512];
            snprintf(request, sizeof(request), "%s\n", command);
            // Send the upload command and filename to the server
            if (write(fd_write_client, request, strlen(request)) == -1) {
                perror("Failed to send upload request to server");
                exit(1);
            }
            // Wait for response from server
            char response[1024];
            memset(response, 0, sizeof(response));
            if (read(fd_read_client, response, sizeof(response)) == -1) {
                perror("Failed to read response from server");
                exit(1);
            }
            printf("Server response:\n%s", response);
        }


        else if (strncmp(command, "download ", 9) == 0) {
            char request[512];
            snprintf(request, sizeof(request), "%s\n", command);
            // Send the download command and filename to the server
            if (write(fd_write_client, request, strlen(request)) == -1) {
                perror("Failed to send download request to server");
                exit(1);
            }
            // Wait for response from server
            char response[1024];
            memset(response, 0, sizeof(response));
            if (read(fd_read_client, response, sizeof(response)) == -1) {
                perror("Failed to read response from server");
                exit(1);
            }
            printf("Server response:\n%s", response);
        }

        
        else if (strcmp(command, "quit") == 0) {
            // Send request to server to write to log file
            if (write(fd_write_client, "quit\n", strlen("quit\n")) == -1) {
                perror("Failed to send write request to server");
                exit(1);
            }
            printf("quit request sent to server.\n");

            // Wait for response from server
            char log_message[256];
            if (read(fd_read_client, log_message, sizeof(log_message)) == -1) {
                perror("Failed to read log message from server");
                exit(1);
            }
            strtok(log_message, "\n");
            printf("Log message from server: %s\n", log_message);

            // Close named pipe and exit
            close(fd_read_client);
            close(fd_write_client);
            exit(0);
        } 
        
        else if (strcmp(command, "killServer") == 0) {
            // Send kill request to server and terminate client program
            if (write(fd_write_client, "killServer\n", strlen("killServer\n")) == -1) {
                perror("Failed to send kill request to server");
                exit(1);
            }
            printf("Kill request sent to server.\n");

            // Close named pipes and exit
            close(fd_read_client);
            close(fd_write_client);
            exit(0);
        }
        
         else {
            printf("Invalid command. Type 'help' for a list of available commands.\n");
        }
    }
    return 0;
}
