#include <arpa/inet.h>  // inet_ntoa
#include <buzzdarray.h> // buzzdarray
#include <fcntl.h>      // fcntl
#include <inttypes.h>   // strtoimax
#include <math.h>       // modf
#include <netinet/in.h> // sockaddr_in
#include <pthread.h>    // for threading , link with lpthread
#include <signal.h>     // signal, sig_atomic_t
#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <string.h>     // memcpy
#include <sys/socket.h> // socket, bind
#include <time.h>       // timespec, nanosleep, clock_gettime
#include <unistd.h>     // read, write, close, sleep

#define LOOP_RATE 10  // cycles per second

#define MAX_LOOP_TIME (1.0 / LOOP_RATE)

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/****************************************/
/****************************************/

/* Outgoing queue code */
typedef struct queue_data_ {
   char *data;
   uint32_t size;
   int originating_socket;
   uint16_t originating_robot_id;
   struct queue_data_* next;
} queue_data;

typedef struct {
   queue_data* first;
   queue_data* last;
} queue;

queue* outgoing_queue;

void outgoing_queue_push(queue_data* queue_data) {
   if(outgoing_queue->last == NULL) { // first element
      outgoing_queue->first = queue_data;
   } else {
      outgoing_queue->last->next = queue_data;
   }
   outgoing_queue->last = queue_data;
   queue_data->next = NULL;
}

queue_data* outgoing_queue_pop() {
   if(outgoing_queue->first == NULL) {
      return NULL;
   }
   queue_data* ret = outgoing_queue->first;
   outgoing_queue->first = outgoing_queue->first->next;
   ret->next = NULL;
   return ret;
}

/* Mutex to be used to read/write to the queue */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/****************************************/
/****************************************/

volatile sig_atomic_t stop_signal;

void interrupt_handler(int signum) {
   stop_signal = 1;
}

typedef struct {
   int client_socket;
   uint16_t robot_id;
} passed_data_t;

/****************************************/
/****************************************/

void *broadcaster_thread(void* clients_) {
   /* loop setup */
   struct timespec ts_start, ts_end, ts_diff;
   double MAX_LOOP_TIME_INTEGRAL;
   const double MAX_LOOP_TIME_FRACTIONAL_NANO = modf(MAX_LOOP_TIME, &MAX_LOOP_TIME_INTEGRAL) * 1e9;
   clock_gettime(CLOCK_MONOTONIC, &ts_start);

   /* Infinite Loop */
   while (!stop_signal) {
      /* Check if any outgoing message is available in queue */
      queue_data* to_send;
      pthread_mutex_lock(&queue_mutex);
      to_send = outgoing_queue_pop();
      pthread_mutex_unlock(&queue_mutex);
      if(to_send != NULL) {
         int64_t clients_length;
         /* Get size of clients */
         pthread_mutex_lock(&clients_mutex);
         buzzdarray_t clients = *(buzzdarray_t*) clients_;
         clients_length = buzzdarray_size(clients);
         pthread_mutex_unlock(&clients_mutex);
         /* If any clients are connected */
         if(clients_length > 0) {
            printf("forwarding %d bytes to all clients\n", to_send->size);
            for (int64_t i = 0; i < clients_length; i++) {
               int client_socket;
               pthread_mutex_lock(&clients_mutex);
               client_socket = buzzdarray_get(clients, i, int);
               pthread_mutex_unlock(&clients_mutex);
               /* If this is not the same client it came from */
               if(client_socket != to_send->originating_socket) {
                  /* Write sender robot id to the client */
                  if (write(client_socket, &to_send->originating_robot_id, sizeof(uint16_t)) == -1) {
                     perror("Cannot write to client");
                     continue;
                  }
                  /* Write size of data(payload) to the client */
                  if (write(client_socket, &to_send->size, sizeof(uint32_t)) == -1) {
                     perror("Cannot write to client");
                     continue;
                  }
                  /* Write data(payload) to the client */
                  if (write(client_socket, to_send->data, to_send->size) == -1) {
                     perror("Cannot write to client");
                     continue;
                  }
               }
            }
         }
         /* clearing queue regardless of any clients connected to the system 
            to keep total memory usage low*/
         free(to_send->data);
         free(to_send);
      }
   /*=================== Loop rate management ================*/
      clock_gettime(CLOCK_MONOTONIC, &ts_end);
      ts_diff.tv_sec = (long)MAX_LOOP_TIME_INTEGRAL - (ts_end.tv_sec - ts_start.tv_sec);
      ts_diff.tv_nsec = (long)MAX_LOOP_TIME_FRACTIONAL_NANO - (ts_end.tv_nsec - ts_start.tv_nsec);

      /* Sleep for the calculated time */
      nanosleep(&ts_diff, NULL);
      clock_gettime(CLOCK_MONOTONIC, &ts_start);
   }
   return 0;
}

/****************************************/
/****************************************/

void usage(const char* path, int status) {
   fprintf(stderr, "Usage:\n\t%s [--port port_number]\n\n", path);
   exit(status);
}

/****************************************/
/****************************************/

/**
 * @brief This will handle connection for each client
 */
void *connection_handler(void *pdata) {
   passed_data_t data = *(passed_data_t *) pdata;

   printf("Connection accepted for RobotID: %d\n", data.robot_id);
   int client_socket = data.client_socket;
   /* Send Robot ID to the connected client */
   if (write(client_socket, &data.robot_id, sizeof(uint16_t)) == -1) {
         perror("Cannot write to client");
         exit(1);
   }
   while (!stop_signal) {
      /* Check if any data is available from client */
      uint16_t received_robot_id;
      ssize_t bytes_read = read(client_socket, &received_robot_id, sizeof(uint16_t));
      if(bytes_read == 0) {
         fprintf(stderr, "Client %d disconnected\n", client_socket);
         pthread_exit(0);
      } else if (bytes_read == -1) {
         continue; // nothing to read
      }
      if(data.robot_id != received_robot_id) {
         perror("Wrong Client ID!! Communication Problem!!\n");
         continue;
      }
      /* Next is the size of incoming data (payload) */
      uint32_t size_of_incoming_data;
      if((bytes_read = read(client_socket, &size_of_incoming_data, sizeof(uint32_t))) == 0) {
         fprintf(stderr, "Client %d disconnected\n", client_socket);
         pthread_exit(0);
      } else if (bytes_read == -1) {
         continue; // nothing to read
      }
      printf("Incoming %d bytes from client %d\n", size_of_incoming_data, data.robot_id);
      /* Create an object to store incoming data */
      char* incoming_data = (char*) malloc(size_of_incoming_data * sizeof(char));
      /* To loop until we read all the info */
      uint32_t total_read = 0;
      char buffer[1024];
      uint32_t to_read_in_loop;
      while (total_read < size_of_incoming_data && !stop_signal) {
         to_read_in_loop = fmin(sizeof(buffer), size_of_incoming_data - total_read);
         bytes_read = read(client_socket, buffer, to_read_in_loop);
         if (bytes_read == 0) {
            fprintf(stderr, "client disconnected");
            pthread_exit(0);
         } else if(bytes_read == -1) {
            perror("No more data to read!");
            break;
         }
         memcpy(incoming_data + total_read, buffer, bytes_read);
         total_read += bytes_read;
      }
      /* Check if we have complete data */
      if (total_read < size_of_incoming_data) {
         fprintf(stderr, "Cannot get the complete message, read %d out of %d\n", total_read, size_of_incoming_data);
         continue;
      }
      queue_data* to_send = (queue_data*) malloc(sizeof(queue_data));
      to_send->originating_socket = client_socket;
      to_send->originating_robot_id = data.robot_id;
      to_send->size = size_of_incoming_data;
      to_send->data = incoming_data;
      outgoing_queue_push(to_send);
   }
   return 0;
}

void join_all_threads(uint32_t pos, void* data, void* params) {
   pthread_t thread_id = * (pthread_t*) data;
   pthread_join(thread_id, NULL);
}

/****************************************/
/****************************************/

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
   /* Create server socket */
   server_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket == -1) {
      perror("socket creation failed");
      exit(1);
   }
   /* Configure to reuse same socket */
   int temp = 1;
   if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp)) == -1) {
      perror("Cannot configure to reuse the same socket");
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

   outgoing_queue = (queue*) malloc(sizeof(queue));
   outgoing_queue->first = NULL;
   outgoing_queue->last = NULL;

   buzzdarray_t threads = buzzdarray_new(10, sizeof(pthread_t), NULL);
   buzzdarray_t clients = buzzdarray_new(10, sizeof(int), NULL);
   pthread_t broadcaster_thread_id;
   /* Broadcsater thread */
   if (pthread_create(&broadcaster_thread_id, NULL, broadcaster_thread, &clients) < 0) {
      perror("could not create thread");
      return 1;
   }
   int client_sock;
   uint16_t robot_ids = 0;
   /* Main Loop */
   /* Check if any new client is trying to connect */
   while (!stop_signal && (client_sock = accept(server_socket, NULL, NULL)) > -1) {
      pthread_t thread_id;
      passed_data_t data = {
         .client_socket = client_sock,
         .robot_id = ++robot_ids
      };

      if (pthread_create(&thread_id, NULL, connection_handler, &data) < 0) {
         perror("could not create thread");
         return 1;
      }
      buzzdarray_push(threads, &thread_id);
      /* append to clients array */
      pthread_mutex_lock(&clients_mutex);
      buzzdarray_push(clients, &client_sock);
      pthread_mutex_unlock(&clients_mutex);
   }
   /* Join all threads */
   buzzdarray_foreach(threads, join_all_threads, NULL);
   pthread_join(broadcaster_thread_id, NULL);
   printf("Closing server...\n");
   buzzdarray_destroy(&threads);
   close(server_socket);
   free(outgoing_queue);
   return 0;
}
