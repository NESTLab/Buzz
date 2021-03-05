// #include<pthread.h> //for threading , link with lpthread
#include <arpa/inet.h>   // inet_ntoa
#include <buzzdarray.h>  // buzzdarray
#include <fcntl.h>       // fcntl
#include <math.h>        // modf
#include <netinet/in.h>  // sockaddr_in
#include <signal.h>      // signal, sig_atomic_t
#include <stdio.h>       // printf
#include <stdlib.h>      // exit
#include <sys/socket.h>  // socket, bind
#include <time.h>        // timespec, nanosleep, clock_gettime
#include <unistd.h>      // read, write, close, sleep

#include <string.h>

#define LOOP_RATE 10  // cycles per second

#define MAX_LOOP_TIME (1.0 / LOOP_RATE)

volatile sig_atomic_t stop_signal;

void interrupt_handler(int signum) {
    printf("Signal Handler %d\n", signum);
    stop_signal = 1;
}

void check_clients(int server_socket, buzzdarray_t clients) {
    int new_sock;

    /* Check if new client is tryi  ng to connect */
    if ((new_sock = accept(server_socket, NULL, NULL)) > -1) {
        printf("Connection accepted : %d\n", new_sock);

        /* Configure the socket in non-blocking mode */
        if (fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL) | O_NONBLOCK) < 0) {
            perror("Cannot configure socket to be in non-blocking mode");
            exit(1);
        }

        buzzdarray_push(clients, &new_sock);
    }

    /* Loop through all clients, check if they are sending
       any data or connection is closed */
    for (int64_t i = 0; i < buzzdarray_size(clients); ++i) {
        const int client_socket = buzzdarray_get(clients, i, int);

        ssize_t read_size;
        const size_t BUFF_SIZE = 1024 * 1024;
        char buf[BUFF_SIZE];

        /* Check if any data is available from client */
        while ((read_size = recv(client_socket, buf, BUFF_SIZE, 0)) > 0) {
            buf[read_size] = '\0';
            printf("Received [%ld bytes]\n", read_size);
        }

        if (read_size == 0) {
            printf("Client disconnected\n");
            // fflush(stdout);
            buzzdarray_remove(clients, i);
        }
        // if (read_size == -1) : No data to receive from client !
    }
}

void send_data(buzzdarray_t clients, char *data, size_t size) {
    for (int64_t i = 0; i < buzzdarray_size(clients); ++i) {
        const int client_socket = buzzdarray_get(clients, i, int);
        if (write(client_socket, data, size) == -1) {
            perror("Cannot write to client");
        }
    }
}

void send_from_stdin(buzzdarray_t clients) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    fd_set savefds = readfds;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    char message[50];

    if (select(1, &readfds, NULL, NULL, &timeout)) {
        scanf("%s", message);
        printf("Sending message [%ld]: %s\n", strlen(message), message);
        send_data(clients, message, strlen(message));
    }

    readfds = savefds;
}

int main(int argc, char *argv[]) {
    int server_socket;
    struct sockaddr_in server;

    /* Prepare the sockaddr_in structure */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    /* Configure signals */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Cannot configure to ignore SIGPIPE");
        exit(1);
    }

    if (signal(SIGINT, interrupt_handler) == SIG_ERR) {
        perror("Cannot configure to listen to interrupt signals");
        exit(1);
    }

    buzzdarray_t clients = buzzdarray_new(10, sizeof(int), NULL);

    /* Create server socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket creation failed");
        exit(1);
    }

    int temp = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp)) == -1) {
        perror("Cannot configure to reuse the same socket");
        exit(1);
    }

    /* Configure the socket to be in non-blocking mode */
    if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK) < 0) {
        perror("Cannot configure socket to be in non-blocking mode");
        exit(1);
    }

    /* Binded newly created socket to given IP and Port */
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) != 0) {
        perror("socket bind failed");
        exit(1);
    }

    /* Now server is ready to listen */
    if (listen(server_socket, 3) != 0) {
        perror("Listen failed");
        exit(1);
    } else {
        printf("Server listening at %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    }

    /* Server loop setup */
    struct timespec ts_start, ts_end, ts_diff;
    double MAX_LOOP_TIME_INTEGRAL;
    const double MAX_LOOP_TIME_FRACTIONAL_NANO = modf(MAX_LOOP_TIME, &MAX_LOOP_TIME_INTEGRAL) * 1e9;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* Main Loop */
    while (!stop_signal) {
        check_clients(server_socket, clients);

        send_from_stdin(clients);

        /*=================== Loop rate management ================*/
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        ts_diff.tv_sec = (long)MAX_LOOP_TIME_INTEGRAL - (ts_end.tv_sec - ts_start.tv_sec);
        ts_diff.tv_nsec = (long)MAX_LOOP_TIME_FRACTIONAL_NANO - (ts_end.tv_nsec - ts_start.tv_nsec);

        /* Sleep for the calculated time */
        nanosleep(&ts_diff, NULL);
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
    }

    printf("Closing server...\n");

    buzzdarray_destroy(&clients);

    close(server_socket);

    return 0;
}
