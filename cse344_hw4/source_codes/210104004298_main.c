#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#define MAX_BUFFER_SIZE 1024 // Maximum number of tasks that can be stored in the buffer

typedef struct { // Structure to hold information about a file copy task
    int src_fd;
    int dest_fd;
    char *src_file_name;
    char *dest_file_name;
} FileTask; // FileTask is a type of struct

FileTask buffer[MAX_BUFFER_SIZE]; // Buffer to store file copy tasks 
int buffer_size; // Size of the buffer (number of tasks that can be stored)
int num_workers; // Number of worker threads
char *src_dir; // Source directory
char *dest_dir; // Destination directory
int buffer_count = 0; // Number of tasks currently in the buffer
int done = 0; // Flag to indicate that all tasks have been added to the buffer and no more tasks will be added

size_t total_bytes_copied = 0; // Total number of bytes copied
int regular_file_count = 0; // Number of regular files
int fifo_file_count = 0; // Number of FIFO files
int directory_count = 0; // Number of directories

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect the buffer and buffer_count variables 
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER; // Condition variable to signal that the buffer is not empty and tasks can be taken from it
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER; // Condition variable to signal that the buffer is not full and tasks can be added to it

void *manager_thread(void *arg);
void *worker_thread(void *arg);
void copy_file(FileTask *task);
void cleanup_file_task(FileTask *task);
void copy_directory(const char *src, const char *dest);
void remove_directory(const char *path);
void print_statistics(struct timeval start_time, struct timeval end_time);
void handle_signal(int signal);

int main(int argc, char *argv[]) { // Main function 
    if (argc != 5) { // Check if the number of arguments is correct 
        fprintf(stderr, "Usage: %s <buffer_size> <num_workers> <src_dir> <dest_dir>\n", argv[0]); // Print usage message if the number of arguments is incorrect
        exit(EXIT_FAILURE); // Exit the program
    }

    signal(SIGINT, handle_signal); // Register the signal handler for SIGINT

    buffer_size = atoi(argv[1]); // Convert the buffer size argument to an integer
    num_workers = atoi(argv[2]); // Convert the number of workers argument to an integer
    src_dir = strdup(argv[3]); // Copy the source directory argument to a new string
    dest_dir = strdup(argv[4]); // Copy the destination directory argument to a new string

    if (buffer_size <= 0 || num_workers <= 0) { // Check if the buffer size and number of workers are greater than 0
        fprintf(stderr, "Buffer size and number of workers must be greater than 0\n"); // Print an error message
        exit(EXIT_FAILURE);
    }

    // Clear destination directory if it exists and create a new empty directory
    remove_directory(dest_dir); // Remove the destination directory if it exists
    mkdir(dest_dir, 0700); // Create a new empty destination directory



    struct timeval start_time, end_time; // Variables to store the start and end times of the program execution
    gettimeofday(&start_time, NULL); // Get the current time as the start time

    pthread_t manager; // Thread for the manager task (copying directories)
    pthread_t workers[num_workers]; // Array of threads for the worker tasks (copying files)

    pthread_create(&manager, NULL, manager_thread, NULL); // Create the manager thread to copy the directories
    for (int i = 0; i < num_workers; i++) { // Create the worker threads to copy the files in parallel
        pthread_create(&workers[i], NULL, worker_thread, NULL); // Create a worker thread
    }

    pthread_join(manager, NULL); // Wait for the manager thread to finish
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    gettimeofday(&end_time, NULL); // Get the current time as the end time

    print_statistics(start_time, end_time); // Print the statistics of the program execution

    free(src_dir); // Free the memory allocated for the source directory string
    free(dest_dir);  // Free the memory allocated for the destination directory string

    return 0;
}

void handle_signal(int signal) { // Signal handler function
    if (signal == SIGINT) {
        fprintf(stderr, "\nProcess interrupted. Cleaning up and exiting...\n"); // Print a message that the process is interrupted and cleaning up
        exit(EXIT_FAILURE);
    }
}

void *manager_thread(void *arg) { // Manager thread function
    copy_directory(src_dir, dest_dir); // Copy the source directory to the destination directory

    pthread_mutex_lock(&buffer_mutex); // Lock the buffer mutex to access the buffer and buffer_count variables
    done = 1;
    pthread_cond_broadcast(&buffer_not_empty); // Signal all worker threads that no more tasks will be added to the buffer
    pthread_mutex_unlock(&buffer_mutex); // Unlock the buffer mutex

    return NULL;
}

void *worker_thread(void *arg) { // Worker thread function
    while (1) { // Infinite loop to keep the worker thread running until all tasks are completed
        pthread_mutex_lock(&buffer_mutex); // Lock the buffer mutex to access the buffer and buffer_count variables

        while (buffer_count == 0 && !done) {
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex); // Wait for the buffer to become non-empty
        }

        if (buffer_count == 0 && done) { // Check if all tasks are completed and no more tasks will be added to the buffer
            pthread_mutex_unlock(&buffer_mutex); // Unlock the buffer mutex
            return NULL;
        }

        FileTask task = buffer[--buffer_count]; // Get the task from the buffer and decrement the buffer count
        pthread_cond_signal(&buffer_not_full); // Signal that the buffer is not full and tasks can be added to it
        pthread_mutex_unlock(&buffer_mutex); // Unlock the buffer mutex

        copy_file(&task); // Copy the file from the source to the destination
        //printf("File %s copied to %s\n", task.src_file_name, task.dest_file_name); // if we want to see the files copied in real time
        cleanup_file_task(&task); // Clean up the file task
    }

    return NULL;
}

void copy_directory(const char *src, const char *dest) { // Function to copy a directory and its contents
    DIR *dir = opendir(src); // Open the source directory
    if (dir == NULL) {
        perror("Failed to open source directory"); // Print an error message if the source directory cannot be opened
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) { // Iterate over the entries in the source directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { // Skip the current and parent directories
            continue;
        }

        char src_path[PATH_MAX];
        char dest_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name); // Construct the full path of the source file
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name); // Construct the full path of the destination file

        if (entry->d_type == DT_DIR) { // Check if the entry is a directory and copy it recursively if it is a directory 
            mkdir(dest_path, 0700); // Create a new directory in the destination directory
            directory_count++;
            copy_directory(src_path, dest_path); // Recursively copy the contents of the directory
        } else {
            pthread_mutex_lock(&buffer_mutex); // Lock the buffer mutex to access the buffer and buffer_count variables

            while (buffer_count == buffer_size) { // Check if the buffer is full and wait for it to become non-full
                pthread_cond_wait(&buffer_not_full, &buffer_mutex);
            }

            FileTask task; // Create a new file task
            task.src_file_name = strdup(src_path); // Copy the source file path to the file task
            task.dest_file_name = strdup(dest_path); // Copy the destination file path to the file task
            task.src_fd = open(task.src_file_name, O_RDONLY); // Open the source file in read-only mode
            task.dest_fd = open(task.dest_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open the destination file in write-only mode, create it if it does not exist, and truncate it

            if (task.src_fd == -1 || task.dest_fd == -1) { // Check if the source or destination file cannot be opened
                perror("File open error"); // Print an error message for file open error
                if (task.src_fd != -1) close(task.src_fd); // Close the source file if it is opened
                if (task.dest_fd != -1) close(task.dest_fd); // Close the destination file if it is opened
                free(task.src_file_name); // Free the memory allocated for the source file name
                free(task.dest_file_name); // Free the memory allocated for the destination file name
                pthread_mutex_unlock(&buffer_mutex); // Unlock the buffer mutex
                continue; // Continue to the next entry in the directory
            }

            buffer[buffer_count++] = task; // Add the file task to the buffer and increment the buffer count

            if (entry->d_type == DT_REG) { // Check if the entry is a regular file
                regular_file_count++; // Increment the regular file count
            } else if (entry->d_type == DT_FIFO) { // Check if the entry is a FIFO file
                fifo_file_count++; // Increment the FIFO file count
            }

            pthread_cond_signal(&buffer_not_empty); // Signal that the buffer is not empty and tasks can be taken from it
            pthread_mutex_unlock(&buffer_mutex); // Unlock the buffer mutex
        }
    }

    closedir(dir); // Close the source directory
}

void copy_file(FileTask *task) { // Function to copy a file from the source to the destination
    char buffer[4096];
    ssize_t bytes;

    while ((bytes = read(task->src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(task->dest_fd, buffer, bytes) != bytes) {
            perror("write"); // Print an error message if the write operation fails 
            break;
        }

        total_bytes_copied += bytes;
    }

    if (bytes == -1) {
        perror("read"); // Print an error message if the read operation fails
    } 

    close(task->src_fd); // Close the source file 
    close(task->dest_fd);   // Close the destination file
} 

void cleanup_file_task(FileTask *task) { // Function to clean up the file task after copying the file
    free(task->src_file_name); // Free the memory allocated for the source file name
    free(task->dest_file_name); // Free the memory allocated for the destination file name
}

void remove_directory(const char *path) { // Function to remove a directory and its contents
    DIR *dir = opendir(path); // Open the directory
    if (dir == NULL) { // Check if the directory cannot be opened 
        return; // Return if the directory cannot be opened
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) { // Iterate over the entries in the directory 
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) { // Check if the entry is a directory and remove it recursively
            remove_directory(full_path);
        } else {
            remove(full_path); 
        }
    }

    closedir(dir); // Close the directory
    rmdir(path); // Remove the directory
}

void print_statistics(struct timeval start_time, struct timeval end_time) { // Function to print the statistics of the program execution
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;      // sec to ms
    elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0;   // us to ms

    printf("\n----------------STATISTICS----------------\n"); 
    printf("Consumers: %d - Buffer Size: %d\n", num_workers, buffer_size);  // Print the number of consumers and buffer size
    printf("Number of Regular Files: %d\n", regular_file_count); // Print the number of regular files
    printf("Number of FIFO Files: %d\n", fifo_file_count); // Print the number of FIFO files
    printf("Number of Directories: %d\n", directory_count);  // Print the number of directories
    printf("TOTAL BYTES COPIED: %zu\n", total_bytes_copied); // Print the total number of bytes copied
    printf("TOTAL TIME: %.3f seconds\n", elapsed_time / 1000.0); // Print the total time taken for the program execution
    printf("------------------------------------------\n");
}
