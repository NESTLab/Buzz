// #include<pthread.h> //for threading , link with lpthread
#include <arpa/inet.h>   // inet_ntoa
#include <buzzdarray.h>  // buzzdarray
#include <fcntl.h>       // fcntl
#include <inttypes.h>    // strtoimax
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
    stop_signal = 1;
}

void check_clients(int server_socket, buzzdarray_t clients) {
    int new_sock;

    /* Check if new client is tryi  ng to connect */
    if ((new_sock = accept(server_socket, NULL, NULL)) > -1) {
        printf("Connection accepted : %d\n", new_sock);
        /* Send Robot ID to the connected client */
        uint16_t robot_id = new_sock;
        if (write(new_sock, &robot_id, sizeof(robot_id)) == -1) {
            perror("Cannot write to client");
            exit(1);
        }
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
        /* Check if any data is available from client */
        uint16_t robot_id;
        ssize_t bytes_read = read(client_socket, &robot_id, sizeof(uint16_t));
        if(bytes_read == 0) {
            fprintf(stderr, "Client %d disconnected\n", client_socket);
            buzzdarray_remove(clients, i);
            continue;
        } else if (bytes_read == -1) {
            continue; // nothing to read
        }
        if(robot_id != client_socket) {
            printf("Wrong Client ID!! Communication Problem!!\n");
            continue;
        }
        /* Next is the size of incoming data (payload) */
        int64_t size_of_incoming_data;
        if((bytes_read = read(client_socket, &size_of_incoming_data, sizeof(int64_t))) == 0) {
            fprintf(stderr, "Client %d disconnected\n", client_socket);
            buzzdarray_remove(clients, i);
            continue;
        } else if (bytes_read == -1) {
            continue; // nothing to read
        }
        printf("Incoming %ld bytes from client %d\n", size_of_incoming_data, robot_id);
        /* Create an object to store incoming data */
        char data[size_of_incoming_data];
        /* To loop until we read all the info */
        int64_t total_read = 0;
        char buffer[1024];
        while (total_read < size_of_incoming_data && !stop_signal) {
            bytes_read = read(client_socket, buffer, sizeof(buffer));
            if (bytes_read == 0) {
                fprintf(stderr, "client disconnected");
                buzzdarray_remove(clients, i);
                break;
            }else if(bytes_read == -1) {
                perror("No more data to read!");
                break;
            }
            memcpy((data + total_read), buffer, bytes_read);
            total_read += bytes_read;
        }
        /* Check if we have complete data */
        if (total_read < size_of_incoming_data) {
            fprintf(stderr, "Cannot get the complete message, read %ld out of %ld\n", total_read, size_of_incoming_data);
            continue;
        }
        printf("forwarding %ld bytes to all clients\n", size_of_incoming_data);
        /* Broadcast it to all clients */
        for (int64_t j = 0; j < buzzdarray_size(clients); ++j) {
            if(i == j) continue; // skip the current client(sender)
            const int new_client_socket = buzzdarray_get(clients, j, int);
            /* Set client socket to blocking */
            if (fcntl(new_client_socket, F_SETFL,  fcntl(new_client_socket, F_GETFL) & ~(O_NONBLOCK)) < 0) {
                perror("Cannot configure socket to be in blocking mode");
                continue;
            }
            /* Write sender robot id to the client */
            if (write(new_client_socket, &robot_id, sizeof(robot_id)) == -1) {
                perror("Cannot write to client");
                continue;
            }
            /* Write size of data(payload) to the client */
            if (write(new_client_socket, &size_of_incoming_data, sizeof(size_of_incoming_data)) == -1) {
                perror("Cannot write to client");
                continue;
            }
            /* Write data(payload) to the client */
            if (write(new_client_socket, data, size_of_incoming_data) == -1) {
                perror("Cannot write to client");
                continue;
            }
            /* Set client socket back to non-blocking */
            if (fcntl(new_client_socket, F_SETFL, fcntl(new_client_socket, F_GETFL) | O_NONBLOCK) < 0) {
                perror("Cannot configure socket to be in non-blocking mode");
                continue;
            }
        }
    }
}

/**
 * @brief print Usage
 * 
 * @param path binary path 
 * @param status exit code
 */
void usage(const char* path, int status) {
   fprintf(stderr, "Usage:\n\t%s [--port port_number]\n\n", path);
   exit(status);
}

int main(int argc, char *argv[]) {
    int server_socket;
    struct sockaddr_in server;

    /* Prepare the sockaddr_in structure */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    if (argc == 1) {
        /* Default port */
        server.sin_port = htons(8080);
    } else if (argc == 3) {
        if(strcmp(argv[1], "--port") == 0) {
            char* end;
            intmax_t val = strtoimax(argv[2], &end, 10);
            if (val <= 0 || val > UINT16_MAX || end == argv[2] || *end != '\0') {
                // Cannot parse as Port
                fprintf(stderr, "Cannot parse Port: %s\n", argv[2]);
                exit(1);
            }
            server.sin_port = htons((uint16_t) val);
        } else {
            fprintf(stderr, "error: %s: unrecognized option '%s'\n", argv[0], argv[1]);
            usage(argv[0], 1);
        }
    } else {
        usage(argv[0], 0);
    }

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
        printf("Server listening on: \033[0;33m %s:%d \033[0m \n Hit CTRL-C to stop the server\n\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    }

    /* Server loop setup */
    struct timespec ts_start, ts_end, ts_diff;
    double MAX_LOOP_TIME_INTEGRAL;
    const double MAX_LOOP_TIME_FRACTIONAL_NANO = modf(MAX_LOOP_TIME, &MAX_LOOP_TIME_INTEGRAL) * 1e9;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* Main Loop */
    while (!stop_signal) {
        check_clients(server_socket, clients);

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
