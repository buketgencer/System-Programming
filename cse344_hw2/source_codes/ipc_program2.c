#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
// sig_atomic_t is in the <signal.h> header file

#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"

volatile sig_atomic_t child_counter = 0;

void sigchld_handler(int signum) {

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child %d finished with status %d\n", pid, WEXITSTATUS(status));
        child_counter++;
    }
}

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int array[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    // Signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        handle_error("Failed to set signal handler");
    }


    // Unlink the FIFOs if they already exist
    unlink(FIFO1);
    unlink(FIFO2);

    // Create the FIFOs
    if (mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1) {
        handle_error("Failed to create FIFOs"); // 0666 is the permission for the FIFOs 
    }

    // Parent process write the array to the first FIFO
    int fd1 = open(FIFO1, O_RDWR);
    int writeReturn = write(fd1, array, sizeof(array)); // write the array to the first FIFO
    if (writeReturn == -1) {
        perror("Failed to write to FIFO1"); // if write fails, print error message
    }
    //close(fd1);

    //Parent process write the array twice to the first FIFO
    int writeReturn2 = write(fd1, array, sizeof(array)); 
    if (writeReturn2 == -1) {
        perror("Failed to write to FIFO1");
    }

    //parent process write the "multiply" string to the second FIFO
    int fd2 = open(FIFO2, O_RDWR);
    char *command = "multiply"; 
    int writeReturn3 = write(fd2, command, strlen(command) + 1); // write the "multiply" string to the second FIFO
    if (writeReturn3 == -1) {
        perror("Failed to write to FIFO2"); // if write fails, print error message
    }
    //close(fd2);

  

    // Fork Child 1
    if (fork() == 0) {

        sleep(10); // Ensure child 1 sleeps before proceeding

        int fd1Child = open(FIFO1, O_RDONLY);
        if(fd1Child == -1) {
            perror("Failed to open FIFO1 in child 1");
        }
        int numbers[10], total = 0;
        //child1 read the array from the first FIFO
        int readReturn = read(fd1Child, numbers, sizeof(numbers));
        if(readReturn == -1) {
            perror("Failed to read from FIFO1 in child 1"); // if read fails, print error message
        }
        close(fd1Child);
        
        // Calculate the sum of the array
        for (int i = 0; i < 10; i++) {
            total += numbers[i];
        }


        // Write the sum to the second FIFO
        int fd2 = open(FIFO2, O_WRONLY); // Open the second FIFO for writing
        write(fd2, &total, sizeof(total));
        close(fd2); // Close the second FIFO
        exit(0); // Exit the child process
    }

    // Fork Child 2
    if (fork() == 0) {

        sleep(10); // Ensure child 2 sleeps before proceeding

        int sum, fd2 = open(FIFO2, O_RDONLY);
        //child2 read the array from the first FIFO
        int arraynumbers[10]; //
        //read the array from first FIFO
         int ReadReturnArray2 = read(fd1, arraynumbers, sizeof(arraynumbers)); //
        if (ReadReturnArray2 == -1) { //
            perror("Failed to read from the first FIFO in child2"); //
        } //
        close(fd1); //


        // read the "multiply" string from the second FIFO
        char multiply[9];
        read(fd2, multiply, sizeof(multiply));
        // check if the string is "multiply"
        if (strcmp(multiply, "multiply") != 0) {
            perror("The value read from the second FIFO is not 'multiply' in child2"); // if the string is not "multiply", print error message
        }
        read(fd2, &sum, sizeof(sum)); // read the sum from the second FIFO
        close(fd2);

        int product = 1;

        // Calculate the product of the array with data from the first FIFO
        for(int i = 0; i < 10; i++) { //
            product *= arraynumbers[i]; //
        } //
   
        //for (int i = 0; i < 10; i++) {
        //    product *= array[i];
        //}
        printf("Sum: %d, Product: %d\n", sum, product);
        exit(0);
    }

    // Wait for children to finish
    //wait(NULL);

    // Additional waitpid to handle any remaining childr
    waitpid(-1, NULL, WNOHANG); // -1 means wait for any child process (if we want to wait for a specific child, we can use the PID of that child instead of -1)
    
    // Wait for children to finish, printing "Proceeding..."
    while (child_counter < 2) {
        printf("Proceeding...\n");
        sleep(2);
    }

    // Close the FIFOs
    if (close(fd1) == -1 || close(fd2) == -1) {
        handle_error("Failed to close FIFOs");
    }
    
    // Unlink the FIFOs after closing them to remove from the system
    unlink(FIFO1);
    unlink(FIFO2);
    printf("Program completed successfully.\n");
    return 0;


}
