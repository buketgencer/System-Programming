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
#include <sys/types.h>
#include <limits.h>

#define MAX_BUFFER_SIZE 1024 // Maximum buffer size

typedef struct { // File task structure
    int src_fd;  // Source file descriptor
    int dest_fd;   // Destination file descriptor
    char *src_file_name; // Source file name
    char *dest_file_name; // Destination file name
} FileTask; // File task structure definition

FileTask buffer[MAX_BUFFER_SIZE]; // Buffer array
int buffer_size; // Buffer size 
int num_workers; // Number of worker threads to create 
char *src_dir; // Source directory
char *dest_dir; // Destination directory
int buffer_count = 0; // Buffer count
int done = 0; // Done flag

size_t total_bytes_copied = 0; // Total bytes copied
int regular_file_count = 0; // Regular file count
int fifo_file_count = 0; // FIFO file count
int directory_count = 0; // Directory count

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER; // Buffer mutex 
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER; // Buffer not empty condition variable
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER; // Buffer not full condition variable
pthread_barrier_t barrier; // Barrier for synchronization 

void *manager_thread(void *arg); // Manager thread function
void *worker_thread(void *arg); // Worker thread function
void copy_file(FileTask *task); // Copy file function
void cleanup_file_task(FileTask *task); // Cleanup file task function
void copy_directory(const char *src, const char *dest); // Copy directory function
void remove_directory(const char *path); // Remove directory function 
void print_statistics(struct timeval start_time, struct timeval end_time); // Print statistics function
void handle_signal(int signal); // Signal handler function

int main(int argc, char *argv[]) { // Main function
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <buffer_size> <num_workers> <src_dir> <dest_dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal); // Register signal handler for SIGINT

    buffer_size = atoi(argv[1]); 
    num_workers = atoi(argv[2]);
    src_dir = strdup(argv[3]);
    dest_dir = strdup(argv[4]);

    if (buffer_size <= 0 || num_workers <= 0) {
        fprintf(stderr, "Buffer size and number of workers must be greater than 0\n"); // Check buffer size and number of workers
        exit(EXIT_FAILURE);
    }

    remove_directory(dest_dir); // Remove destination directory if it exists
    mkdir(dest_dir, 0700); // Create destination directory with read, write and execute permissions for owner

    struct timeval start_time, end_time; // Start and end time for statistics calculation
    gettimeofday(&start_time, NULL); // Get start time

    pthread_t manager; // Manager thread to copy directory
    pthread_t workers[num_workers]; // Worker threads to copy files

    pthread_barrier_init(&barrier, NULL, num_workers + 1); // Initialize barrier with number of workers + 1

    pthread_create(&manager, NULL, manager_thread, NULL); // Create manager thread
    for (int i = 0; i < num_workers; i++) { // Create worker threads
        pthread_create(&workers[i], NULL, worker_thread, NULL); 
    }   

    pthread_barrier_wait(&barrier); // Initial barrier wait to ensure all threads are ready
    pthread_barrier_destroy(&barrier); // Destroy barrier after use

    pthread_join(manager, NULL); // Wait for manager thread to finish
    for (int i = 0; i < num_workers; i++) { // Wait for worker threads to finish
        pthread_join(workers[i], NULL);     
    }

    gettimeofday(&end_time, NULL); // Get end time
  
    print_statistics(start_time, end_time); // Print statistics

    free(src_dir); // Free source directory
    free(dest_dir); // Free destination directory

    return 0; // Exit successfully
}

void handle_signal(int signal) { // Signal handler function
    if (signal == SIGINT) {
        fprintf(stderr, "\nProcess interrupted. Cleaning up and exiting...\n");
        exit(EXIT_FAILURE);
    }
}

void *manager_thread(void *arg) { // Manager thread function
    copy_directory(src_dir, dest_dir); // Copy source directory to destination directory

    pthread_mutex_lock(&buffer_mutex); // Lock buffer mutex
    done = 1; // Set done flag
    pthread_cond_broadcast(&buffer_not_empty); // Broadcast buffer not empty condition variable
    pthread_mutex_unlock(&buffer_mutex); // Unlock buffer mutex

    return NULL; // Return NULL to exit thread
}

void *worker_thread(void *arg) { // Worker thread function
    pthread_barrier_wait(&barrier); // Ensure all worker threads are ready before starting

    while (1) {
        pthread_mutex_lock(&buffer_mutex); // Lock buffer mutex

        while (buffer_count == 0 && !done) { // Wait while buffer is empty and not done
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex); // Wait for buffer not empty condition variable
        }

        if (buffer_count == 0 && done) { // If buffer is empty and done
            pthread_mutex_unlock(&buffer_mutex); // Unlock buffer mutex
            return NULL; // Return NULL to exit thread
        }

        FileTask task = buffer[--buffer_count]; // Get file task from buffer
        pthread_cond_signal(&buffer_not_full);  // Signal buffer not full condition variable
        pthread_mutex_unlock(&buffer_mutex); // Unlock buffer mutex

        copy_file(&task); // Copy file
        cleanup_file_task(&task); // Cleanup file task
    }

    return NULL;
}
 
void copy_directory(const char *src, const char *dest) { // Copy directory function
    DIR *dir = opendir(src);
    if (dir == NULL) {
        perror("Failed to open source directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dest_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        if (entry->d_type == DT_DIR) { // If entry is a directory
            mkdir(dest_path, 0700); // Create directory with read, write and execute permissions for owner
            directory_count++;
            copy_directory(src_path, dest_path); // Copy directory recursively
        } else {
            pthread_mutex_lock(&buffer_mutex); // Lock buffer mutex to add file task to buffer

            while (buffer_count == buffer_size) { // Wait while buffer is full
                pthread_cond_wait(&buffer_not_full, &buffer_mutex); // Wait for buffer not full condition variable to add file task
            }

            FileTask task; // File task to copy file
            task.src_file_name = strdup(src_path); // Source file name
            task.dest_file_name = strdup(dest_path); // Destination file name
            task.src_fd = open(task.src_file_name, O_RDONLY); // Open source file with read only mode
            task.dest_fd = open(task.dest_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open destination file with write only, create and truncate mode

            if (task.src_fd == -1 || task.dest_fd == -1) {  // Check if file open failed
                perror("File open error"); // Print error message
                if (task.src_fd != -1) close(task.src_fd); // Close source file descriptor if open
                if (task.dest_fd != -1) close(task.dest_fd); // Close destination file descriptor if open
                free(task.src_file_name); // Free source file name
                free(task.dest_file_name); // Free destination file name 
                pthread_mutex_unlock(&buffer_mutex); // Unlock buffer mutex
                continue;
            }

            buffer[buffer_count++] = task; // Add file task to buffer

            if (entry->d_type == DT_REG) { // Check if entry is a regular file
                regular_file_count++;
            } else if (entry->d_type == DT_FIFO) { // Check if entry is a FIFO file
                fifo_file_count++;
            }

            pthread_cond_signal(&buffer_not_empty); // Signal buffer not empty condition variable
            pthread_mutex_unlock(&buffer_mutex); // Unlock buffer mutex
        }
    }

    closedir(dir); // Close directory after reading
}

void copy_file(FileTask *task) { // Copy file function
    char buffer[4096];
    ssize_t bytes;

    while ((bytes = read(task->src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(task->dest_fd, buffer, bytes) != bytes) {
            perror("write"); // Print error message if write failed
            break;
        }

        total_bytes_copied += bytes; // Increment total bytes copied by bytes read from source file
    }

    if (bytes == -1) {
        perror("read"); // Print error message if read failed
    }

    close(task->src_fd);  // Close source file descriptor
    close(task->dest_fd); // Close destination file descriptor
}
 
void cleanup_file_task(FileTask *task) { // Cleanup file task function
    free(task->src_file_name); // Free source file name
    free(task->dest_file_name); // Free destination file name
}

void remove_directory(const char *path) { // Remove directory function
    DIR *dir = opendir(path); // Open directory
    if (dir == NULL) { // Check if directory open failed
        return; 
    }

    struct dirent *entry; // Directory entry
    while ((entry = readdir(dir)) != NULL) { // Read directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { // Skip . and .. entries
            continue;
        }

        char full_path[PATH_MAX]; // Full path of entry
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); // Create full path

        if (entry->d_type == DT_DIR) { // If entry is a directory
            remove_directory(full_path); // Remove directory recursively
        } else {
            remove(full_path); // Remove file
        }
    }

    closedir(dir); // Close directory after reading
    rmdir(path);    // Remove directory
} 

void print_statistics(struct timeval start_time, struct timeval end_time) { // Print statistics function
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("\n----------------STATISTICS----------------\n"); // Print statistics
    printf("Consumers: %d - Buffer Size: %d\n", num_workers, buffer_size);
    printf("Number of Regular Files: %d\n", regular_file_count);
    printf("Number of FIFO Files: %d\n", fifo_file_count);
    printf("Number of Directories: %d\n", directory_count);
    printf("TOTAL BYTES COPIED: %zu\n", total_bytes_copied);
    printf("TOTAL TIME: %.3f seconds\n", elapsed_time / 1000.0);
    printf("------------------------------------------\n");
}
