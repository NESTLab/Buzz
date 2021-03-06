#include <arpa/inet.h>   // inet_addr
#include <fcntl.h>       // fcntl
#include <math.h>        // modf
#include <netinet/in.h>  // sockaddr_in
#include <signal.h>      // signal, sig_atomic_t
#include <stdio.h>       // printf
#include <stdlib.h>      // exit
#include <string.h>      // strlen
#include <sys/socket.h>  // connect
#include <time.h>        // timespec, nanosleep, clock_gettime
#include <unistd.h>      // read, write, close

#define LOOP_RATE 10  // cycles per second

#define MAX_LOOP_TIME (1.0 / LOOP_RATE)

volatile sig_atomic_t stop_signal;

void interrupt_handler(int signum) {
    printf("Signal Handler %d\n", signum);
    stop_signal = 1;
}

void read_from_server(int server_socket) {
    char buff[1024 * 1024];
    ssize_t bytesRead;

    if ((bytesRead = read(server_socket, buff, sizeof(buff))) > 0) {
        buff[bytesRead] = '\0';
        printf("From Server[%ld bytes]\n", bytesRead);
    }
}

void send_to_server(int server_socket, void *data, size_t size) {
    if (write(server_socket, data, size) == -1) {
        perror("Cannot write to server");
        stop_signal = 1;
    }
}

void send_from_stdin(int server_socket) {
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
        
        uint16_t v = 10; // Robot ID
        send_to_server(server_socket, &v, sizeof(v));

        v = strlen(message); // Number of messages
        send_to_server(server_socket, &v, sizeof(v));

        send_to_server(server_socket, message, strlen(message));
    }

    readfds = savefds;
}

int main(int argc, char *argv[]) {
    int server_socket;
    struct sockaddr_in server;

    /* Prepare the sockaddr_in structure */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8080);

    /* Configure signals */
    if (signal(SIGINT, interrupt_handler) == SIG_ERR) {
        perror("Cannot configure to listen to interrupt signals");
        exit(1);
    }
    if (signal(SIGPIPE, interrupt_handler) == SIG_ERR) {
        perror("Cannot configure to stop at SIGPIPE");
        exit(1);
    }

    /* Create server socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("socket creation failed...\n");
        exit(1);
    }

    /* Connect to server socket */
    if (connect(server_socket, (struct sockaddr_in *)&server, sizeof(server)) != 0) {
        perror("connection with the server failed !");
        exit(1);
    } else {
        printf("connected to the server..\n");
    }

    /* Configure the socket to be in non-blocking mode */
    if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK) < 0) {
        perror("Cannot configure socket to be in non-blocking mode");
        exit(1);
    }

    /* Client loop setup */
    struct timespec ts_start, ts_end, ts_diff;
    double MAX_LOOP_TIME_INTEGRAL;
    const double MAX_LOOP_TIME_FRACTIONAL_NANO = modf(MAX_LOOP_TIME, &MAX_LOOP_TIME_INTEGRAL) * 1e9;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* Main Loop */
    while (!stop_signal) {
        read_from_server(server_socket);

        send_from_stdin(server_socket);

        /*=================== Loop rate management ================*/
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        ts_diff.tv_sec = (long)MAX_LOOP_TIME_INTEGRAL - (ts_end.tv_sec - ts_start.tv_sec);
        ts_diff.tv_nsec = (long)MAX_LOOP_TIME_FRACTIONAL_NANO - (ts_end.tv_nsec - ts_start.tv_nsec);

        /* Sleep for the calculated time */
        nanosleep(&ts_diff, NULL);
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
    }

    printf("closing \n");
    
    /* close the socket */
    close(server_socket);
}
