#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct {
    int client_socket;
    int order_id;
} ClientArgs;

void *read_status(void *arg) {
    ClientArgs *client_args = (ClientArgs *)arg;
    int client_socket = client_args->client_socket;
    int order_id = client_args->order_id;
    free(client_args);

    char response[256];
    int response_length;
    while ((response_length = read(client_socket, response, 255)) > 0) {
        response[response_length] = '\0';
        printf("Order %d: %s", order_id, response);
    }
    close(client_socket);
    return NULL;
}

void send_order(const char *ip, int port, int num_clients, int p, int q) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[256];

    pthread_t threads[num_clients];

    for (int i = 0; i < num_clients; i++) {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0) {
            perror("Socket could not be created");
            exit(EXIT_FAILURE);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);

        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed log file is written");
            close(client_socket);
            continue;
        }

        int customer_x = rand() % p;
        int customer_y = rand() % q;

        sprintf(buffer, "NEW_ORDER %d %d %d", i, customer_x, customer_y);
        write(client_socket, buffer, strlen(buffer));

        // Create a thread to read status updates
        ClientArgs *client_args = malloc(sizeof(ClientArgs));
        client_args->client_socket = client_socket;
        client_args->order_id = i;
        pthread_create(&threads[i], NULL, read_status, (void *)client_args);
    }

    // Join threads
    for (int i = 0; i < num_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    // Notify the server that all orders are received
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket could not be created");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed. log file is written");
        close(client_socket);
        return;
    }

    write(client_socket, "ALL_ORDERS_RECEIVED", strlen("ALL_ORDERS_RECEIVED"));
    printf("All customers served.\n");
    close(client_socket);

    
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s [ip] [portnumber] [numberOfClients] [p] [q]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int num_clients = atoi(argv[3]);
    int p = atoi(argv[4]);
    int q = atoi(argv[5]);

    send_order(ip, port, num_clients, p, q);

    // Wait for the server to finish processing all orders
    sleep(1);
    //terminate the client program after all orders are received

    return 0;
}
