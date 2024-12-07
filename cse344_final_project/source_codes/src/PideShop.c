#include "../includes/pide_shop.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Global variables
Cook *cooks;
DeliveryPerson *delivery_persons;
Oven oven;
int cook_thread_pool_size;
int delivery_thread_pool_size;
int delivery_speed;
int server_socket;
FILE *log_file;
pthread_mutex_t order_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t order_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_orders_cond = PTHREAD_COND_INITIALIZER;
Order *orders;
int order_count = 0;
int all_orders_received = 0;
int orders_completed = 0;
int ready_count = 0;

typedef struct {
    Order order;
    int client_socket;
} OrderArgs;

// Function declarations
void inform_client(int client_socket, const char *status);

void init_shop(int cook_pool_size, int delivery_pool_size, int delivery_speed_param) {
    cook_thread_pool_size = cook_pool_size;
    delivery_thread_pool_size = delivery_pool_size;
    delivery_speed = delivery_speed_param;

    cooks = malloc(sizeof(Cook) * cook_thread_pool_size);
    if (cooks == NULL) {
        perror("Memory allocation failed for cooks");
        exit(EXIT_FAILURE);
    }

    delivery_persons = malloc(sizeof(DeliveryPerson) * delivery_thread_pool_size);
    if (delivery_persons == NULL) {
        perror("Memory allocation failed for delivery persons");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&oven.oven_sem, 0, MAX_OVEN_CAPACITY) != 0) {
        perror("Oven semaphore could not be initialized");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&oven.spatula_sem, 0, MAX_SPATULAS * SPATULAS_PER_OVEN_ENTRY) != 0) {
        perror("Spatula semaphore could not be initialized");
        exit(EXIT_FAILURE);
    }
    oven.current_capacity = 0;

    for (int i = 0; i < cook_thread_pool_size; i++) {
        cooks[i].cook_id = i + 1;
        cooks[i].is_busy = 0;
        if (pthread_create(&cooks[i].thread, NULL, cook_function, (void *)&cooks[i]) != 0) {
            perror("Cook thread could not be created");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < delivery_thread_pool_size; i++) {
        delivery_persons[i].delivery_id = i + 1;
        delivery_persons[i].is_busy = 0;
        if (pthread_create(&delivery_persons[i].thread, NULL, delivery_function, (void *)&delivery_persons[i]) != 0) {
            perror("Delivery thread could not be created");
            exit(EXIT_FAILURE);
        }
    }

    log_file = fopen("pide_shop.log", "w");
    if (!log_file) {
        perror("Log file could not be opened");
        exit(EXIT_FAILURE);
    }

    orders = malloc(sizeof(Order) * 200);  // Allocate memory for a maximum of 100 orders
    if (!orders) {
        perror("Memory allocation failed for orders");
        exit(EXIT_FAILURE);
    }

}

void start_server(const char *ip, int port) {
    struct sockaddr_in server_addr;
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket could not be created");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind operation failed for server socket. Port may be in use.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen operation failed. Server may be busy.");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    log_event("PideShop active waitng for connection …");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept operation failed. Client connection could not be accepted.");
            continue;
        }
        printf("New connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        pthread_t client_thread;
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)client_sock_ptr) != 0) {
            perror("Client thread could not be created. Connection will be closed.");
            close(client_socket);
            free(client_sock_ptr);
        }
        
        pthread_detach(client_thread);  // Ensure resources are cleaned up when thread finishes
    }
}

void cancel_order(int order_id) {
    pthread_mutex_lock(&order_mutex);
    for (int i = 0; i < order_count; i++) {
        if (orders[i].order_id == order_id) {
            orders[i].status = ORDER_CANCELLED;
            log_event("Order cancelled");
            break;
        }
    }
    pthread_mutex_unlock(&order_mutex);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[256];
    int n = read(client_socket, buffer, 255);
    if (n < 0) {
        perror("Error reading from socket");
        close(client_socket);
        return NULL;
    }

    buffer[n] = '\0';
    log_event(buffer);

    if (strncmp(buffer, "NEW_ORDER", 9) == 0) {
        Order new_order;
        sscanf(buffer, "NEW_ORDER %d %d %d", &new_order.order_id, &new_order.customer_x, &new_order.customer_y);
        new_order.status = ORDER_PLACED;
        new_order.client_socket = client_socket;  // Set client_socket for the order

        char event[256];
        sprintf(event, "Order %d received: PLACED", new_order.order_id);
        log_event(event);
        inform_client(client_socket, "PLACED");

        pthread_mutex_lock(&order_mutex);
        orders[order_count] = new_order;
        order_count++;
        pthread_cond_signal(&order_cond);
        pthread_mutex_unlock(&order_mutex);
    } else if (strcmp(buffer, "ALL_ORDERS_RECEIVED") == 0) {
        pthread_mutex_lock(&order_mutex);
        all_orders_received = 1;
        pthread_cond_broadcast(&order_cond);
        pthread_mutex_unlock(&order_mutex);
    } else if (strncmp(buffer, "CANCEL_ORDER", 12) == 0) {
        int order_id;
        sscanf(buffer, "CANCEL_ORDER %d", &order_id);
        cancel_order(order_id);
        inform_client(client_socket, "CANCELLED");
    }

    // Do not close the client socket here; let the order completion handle it

    return NULL;
}

void *cook_function(void *arg) {
    Cook *cook = (Cook *)arg;
    char event[256];
    sprintf(event, "Cook %d started.", cook->cook_id);
    log_event(event);

    while (1) {
        pthread_mutex_lock(&order_mutex);
        while (order_count == 0 && !all_orders_received) {
            pthread_cond_wait(&order_cond, &order_mutex);
        }
        if (order_count == 0 && all_orders_received) {
            pthread_mutex_unlock(&order_mutex);
            break;
        }

        order_count--;
        Order order = orders[order_count];
        pthread_mutex_unlock(&order_mutex);

        sprintf(event, "Order %d is being prepared by cook %d.", order.order_id, cook->cook_id);
        log_event(event);
        inform_client(order.client_socket, "PREPARING");

        int preparation_time = rand() % 2 + 1;
        int cooking_time = preparation_time / 2;

        sleep(preparation_time);
        order.status = ORDER_PREPARING;
        sprintf(event, "Order %d prepared: PREPARED", order.order_id);
        log_event(event);
        inform_client(order.client_socket, "PREPARED");

        sem_wait(&oven.spatula_sem);
        sprintf(event, "Order %d took a spatula.", order.order_id);
        log_event(event);

        sem_wait(&oven.oven_sem);
        oven.current_capacity++;
        sprintf(event, "Oven has %d slots left.", MAX_OVEN_CAPACITY - oven.current_capacity);
        log_event(event);

        sleep(cooking_time);

        oven.current_capacity--;
        sem_post(&oven.oven_sem);

        order.status = ORDER_COOKING;
        sprintf(event, "Order %d is cooking: COOKED", order.order_id);
        log_event(event);
        inform_client(order.client_socket, "COOKED");

        sleep(cooking_time);

        order.status = ORDER_READY;
        sprintf(event, "Order %d is ready for delivery: READY", order.order_id);
        log_event(event);
        inform_client(order.client_socket, "READY");

        pthread_mutex_lock(&order_mutex);
        ready_count++;
        if (ready_count == order_count && all_orders_received) {
            pthread_cond_signal(&all_orders_cond);
        }
        pthread_mutex_unlock(&order_mutex);

        sem_post(&oven.spatula_sem);
    }

    return NULL;
}

void *delivery_function(void *arg) {
    DeliveryPerson *delivery_person = (DeliveryPerson *)arg;
    char event[256];
    sprintf(event, "Delivery person %d started.", delivery_person->delivery_id);
    log_event(event);

    while (1) {
        pthread_mutex_lock(&order_mutex);
        while (order_count == 0 && !all_orders_received) {
            pthread_cond_wait(&order_cond, &order_mutex);
        }
        if (order_count == 0 && all_orders_received) {
            pthread_mutex_unlock(&order_mutex);
            break;
        }

        // Find an order to deliver
        Order order_to_deliver = {0};
        for (int i = 0; i < order_count; i++) {
            if (orders[i].status == ORDER_READY) {
                order_to_deliver = orders[i];
                orders[i].status = ORDER_HANDING_OUT;
                break;
            }
        }
        pthread_mutex_unlock(&order_mutex);

        if (order_to_deliver.order_id != 0) {
            sprintf(event, "Order %d is being delivered by delivery person %d.", order_to_deliver.order_id, delivery_person->delivery_id);
            log_event(event);
            inform_client(order_to_deliver.client_socket, "DELIVERING");

            int distance = abs(order_to_deliver.customer_x - (10 / 2)) + abs(order_to_deliver.customer_y - (10 / 2));
            sleep(distance / delivery_speed);

            sprintf(event, "Order %d completed.", order_to_deliver.order_id);
            log_event(event);
            inform_client(order_to_deliver.client_socket, "COMPLETED");

            close(order_to_deliver.client_socket); // Close the client socket after sending all statuses

            pthread_mutex_lock(&order_mutex);
            orders_completed++;
            if (orders_completed == ready_count && all_orders_received) {
                pthread_cond_signal(&all_orders_cond);
            }
            pthread_mutex_unlock(&order_mutex);
        }
    }

    return NULL;
}

void log_event(const char *event) {
    if (log_file != NULL) {
        fprintf(log_file, " %s\n", event);
        fflush(log_file);
    } else {
        fprintf(stderr, "Log file is not open.\n");
    }
    fprintf(stderr, " %s\n", event);
}

void inform_client(int client_socket, const char *status) {
    char buffer[256];
    sprintf(buffer, "STATUS %s\n", status);
    write(client_socket, buffer, strlen(buffer));
}

void signal_handler(int signum) {
    log_event(" .. Upps quiting.. writing log file");
    fclose(log_file);
    free(cooks);
    free(delivery_persons);
    free(orders);
    close(server_socket);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s [ip] [portnumber] [CookthreadPoolSize] [DeliveryPoolSize] [k]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    log_file = fopen("event_log.txt", "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    cook_thread_pool_size = atoi(argv[3]);
    delivery_thread_pool_size = atoi(argv[4]);
    delivery_speed = atoi(argv[5]);

    signal(SIGINT, signal_handler);

    init_shop(cook_thread_pool_size, delivery_thread_pool_size, delivery_speed);
    start_server(ip, port);

    // Wait for all orders to complete
    pthread_mutex_lock(&order_mutex);
    while (orders_completed < ready_count || !all_orders_received) {
        pthread_cond_wait(&all_orders_cond, &order_mutex);
    }
    pthread_mutex_unlock(&order_mutex);

    for (int i = 0; i < cook_thread_pool_size; i++) {
        pthread_join(cooks[i].thread, NULL);
    }

    for (int i = 0; i < delivery_thread_pool_size; i++) {
        pthread_join(delivery_persons[i].thread, NULL);
    }

    log_event("All orders are completed. Server is shutting down. Log file is written.");
    fclose(log_file);
    free(cooks);
    free(delivery_persons);
    free(orders);
    close(server_socket);
    return 0;
}
