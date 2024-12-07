#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>  // sleep fonksiyonu için gerekli
#include <signal.h>

#define MAX_AUTOMOBILES 8
#define MAX_PICKUPS 4

sem_t newAutomobile;
sem_t inChargeforAutomobile;
sem_t newPickup;
sem_t inChargeforPickup;

volatile sig_atomic_t keepRunning = 1; // when the signal is received, this variable will be set to 0. ctrl+c will be used to send the signal

int mFree_automobile = MAX_AUTOMOBILES;
int mFree_pickup = MAX_PICKUPS;

void* automobileOwner(void* arg);
void* automobileAttendant(void* arg);
void* pickupOwner(void* arg);
void* pickupAttendant(void* arg);
void* createVehicle(void* arg);
void signalHandler(int sig);




void signalHandler(int sig) { // Signal handler function
    printf("Received signal %d, stopping threads...\n", sig); // Print the signal number
    keepRunning = 0; // Set the flag to stop the threads

    // Wake up the threads that are waiting on semaphores
    sem_post(&newAutomobile);
    sem_post(&newPickup);
    sem_post(&inChargeforAutomobile);
    sem_post(&inChargeforPickup);
}

void setupSignalHandling() { // Signal handling function
    struct sigaction sa; // Signal action struct to set up signal handling
    sa.sa_handler = signalHandler; // Set the signal handler function to be called when a signal is received
    sigemptyset(&sa.sa_mask); // Set the signal mask to empty to allow all signals to be received
    sa.sa_flags = 0; // Set the flags to 0
    sigaction(SIGINT, &sa, NULL); // Set up the signal handling for SIGINT (Ctrl+C) signal  // SIGINT is the signal for Ctrl+C
}

void* automobileOwner(void* arg) {
    struct timespec ts; // timespec struct to hold the time for the semaphore timeout
    while (keepRunning) {
        printf("                                                         | o-o   : Automobile owner arrived.\n");
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5; // Wait for 5 seconds for a spot to be available
       if (sem_timedwait(&newAutomobile, &ts) != 0) {
            continue;  // If semaphore wait times out, retry
        }
        if (mFree_automobile > 0) {
            printf("                                                         | o-o   : Temporary spot available for automobile. Parking now.\n");
            mFree_automobile--; // Decrement the number of free spots for automobile
            //print the number of free spots
            printf("                                                         | N     : Number of free spots for automobile: %d\n", mFree_automobile);
            sem_post(&inChargeforAutomobile); // Release the semaphore for the attendant
        } else {
            printf("                                                         | X     : No temporary spot available for automobile. Exiting.\n");
        }
        sem_post(&newAutomobile); // Release the semaphore for the next car
        sleep(5); // Simulate time taken for the next car to arrive
    }
    pthread_exit(NULL); // Exit the thread 
}

void* automobileAttendant(void* arg) { // Automobile attendant function
    struct timespec ts; // timespec struct to hold the time for the semaphore timeout
    while (keepRunning) {
        printf("                                                         | :) R  : Automobile attendant ready.\n");
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5; // Wait for 5 seconds for a car to be parked
        if (sem_timedwait(&inChargeforAutomobile, &ts) != 0) {
            continue; // If semaphore wait times out, retry
        }
        printf("                                                         | :)    : Automobile attendant parks the automobile.\n");
        mFree_automobile++; // Increment the number of free spots
        sem_post(&newAutomobile); // Release the semaphore for the next car
        sleep(1); // Simulate time taken to park a car
    }
}

void* pickupOwner(void* arg) {
    struct timespec ts; // timespec struct to hold the time for the semaphore timeout
    while (keepRunning) { // Keep running until the signal is received
        printf("o-o^  : Pickup owner arrived.                            |\n"); // Print the message for the pickup owner
         clock_gettime(CLOCK_REALTIME, &ts); // Get the current time
        ts.tv_sec += 5; // Wait for 5 seconds for a spot to be available
        if (sem_timedwait(&newPickup, &ts) != 0) {
            continue; // If semaphore wait times out, retry
        }
        if (mFree_pickup > 0) {
            printf("o-o^  : Temporary spot available for pickup. Parking now.|\n");
            mFree_pickup--; // Decrement the number of free spots for pickup
            //print the number of free spots
            printf("N^    : Number of free spots for pickup: %d               |\n", mFree_pickup);
            sem_post(&inChargeforPickup); // Release the semaphore for the attendant
        } else {
            printf("X^    : No temporary spot available for pickup. Exiting. |\n");
        }
        sem_post(&newPickup); // Release the semaphore for the next pickup
        sleep(5); // Simulate time taken for the next pickup to arrive
    }
    pthread_exit(NULL); // Exit the thread
}

void* pickupAttendant(void* arg) { // Pickup attendant function
    struct timespec ts; // timespec struct to hold the time for the semaphore timeout
    while (keepRunning) { // Keep running until the signal is received
        printf("^:) R : Pickup attendant ready.                          |\n");
        clock_gettime(CLOCK_REALTIME, &ts); // Get the current time
        ts.tv_sec += 5; // Wait for 5 seconds for a car to be parked
        if (sem_timedwait(&inChargeforPickup, &ts) != 0) { // Wait for the semaphore with a timeout
            continue; // If semaphore wait times out, retry
        }

        printf("^:)   : Pickup attendant parks the pickup.               |\n");
        mFree_pickup++; // Increment the number of free spots
        sem_post(&newPickup); // Release the semaphore for the next pickup
        sleep(1); // Simulate time taken to park a car
    }
}

void* createVehicle(void* arg) { // Vehicle creation function

    while (keepRunning) {
        int vehicleType = rand() % 100; // randomly determine the vehicle type 
        pthread_t thread; // Thread variable to hold the thread ID
        if (vehicleType % 2 == 0) {
            pthread_create(&thread, NULL, pickupOwner, NULL); // Create a new thread for the pickup owner
        } else {
            pthread_create(&thread, NULL, automobileOwner, NULL); // Create a new thread for the automobile owner
        }
        pthread_detach(thread); // Detach the thread to allow it to run independently
        sleep(1); // Sleep for 1 second to create the next vehicle
    }
}




int main() {

    setupSignalHandling(); // Set up signal handling
    srand(time(NULL));

    // Initialize the semaphores for threads to wait on and signal each other
    sem_init(&newAutomobile, 0, 1); 
    sem_init(&inChargeforAutomobile, 0, 0);
    sem_init(&newPickup, 0, 1);
    sem_init(&inChargeforPickup, 0, 0);

    pthread_t autoAttendantThread, pickupAttendantThread, vehicleCreatorThread; // Thread variables to hold the thread IDs
    pthread_create(&autoAttendantThread, NULL, automobileAttendant, NULL); // Create a new thread for the automobile attendant
    pthread_create(&pickupAttendantThread, NULL, pickupAttendant, NULL); // Create a new thread for the pickup attendant
    pthread_create(&vehicleCreatorThread, NULL, createVehicle, NULL);  // Create a new thread for the vehicle creator

   // pthread_join is used to wait for the threads to finish before exiting the program
    pthread_join(autoAttendantThread, NULL); // Wait for the automobile attendant thread to finish
    pthread_join(pickupAttendantThread, NULL); // Wait for the pickup attendant thread to finish
    pthread_join(vehicleCreatorThread, NULL);  // Wait for the vehicle creator thread to finish

    // Clean up the semaphores
    sem_destroy(&newAutomobile);
    sem_destroy(&inChargeforAutomobile);
    sem_destroy(&newPickup);
    sem_destroy(&inChargeforPickup);

    printf("Cleaned up and exiting.\n"); // write the message to the console to indicate that the program is exiting

    return 0;
}
