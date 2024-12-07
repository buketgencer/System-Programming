#ifndef PIDE_SHOP_H
#define PIDE_SHOP_H

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define MAX_OVEN_CAPACITY 6
#define MAX_SPATULAS 3
#define SPATULAS_PER_OVEN_ENTRY 2  // Her fırıncı küreği maksimum iki sipariş alabilir

typedef enum {
    ORDER_PLACED,
    ORDER_PREPARING,
    ORDER_COOKING,
    ORDER_READY,
    ORDER_HANDING_OUT,
    ORDER_CANCELLED
} OrderStatus;

typedef struct {
    int order_id;
    OrderStatus status;
    int customer_x;
    int customer_y;
    int client_socket; // Added client_socket to track the client connection
} Order;

typedef struct {
    int cook_id;
    pthread_t thread;
    int is_busy;
} Cook;

typedef struct {
    int delivery_id;
    pthread_t thread;
    int is_busy;
} DeliveryPerson;

typedef struct {
    sem_t oven_sem;         // Fırın kapasitesini kontrol eder
    int current_capacity;
    sem_t spatula_sem;      // Fırıncı küreklerini kontrol eder
} Oven;

extern Cook *cooks;
extern DeliveryPerson *delivery_persons;
extern Oven oven;
extern int cook_thread_pool_size;
extern int delivery_thread_pool_size;
extern int delivery_speed;
extern int server_socket;
extern FILE *log_file;
extern pthread_mutex_t order_mutex;
extern pthread_cond_t order_cond;
extern Order *orders;
extern int order_count;
extern int all_orders_received;

void init_shop(int cook_pool_size, int delivery_pool_size, int delivery_speed);
void start_server(const char *ip, int port);
void *handle_client(void *arg);  // Updated to match pthread_create signature
void *cook_function(void *arg);
void *delivery_function(void *arg);
void log_event(const char *event);
void signal_handler(int signum);
void *process_order(void *arg);

#endif // PIDE_SHOP_H
