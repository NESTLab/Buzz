#include <arpa/inet.h>
#include <buzz/buzzasm.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>


/* time duration for each cycle in the loop (usually 0.1 seconds) */
struct timespec LOOP_TIME_STEP = { 0, 0.1 * 1e9}; // 0.1 seconds

#define check_arg(arg)                                                                 \
   if(strcmp(arg, "--trace") == 0) {                                                   \
      trace = 1;                                                                       \
   } else if(strcmp(arg, "--server") == 0) {                                           \
      server = 1;                                                                      \
   }                                                                                   \
   else {                                                                              \
      fprintf(stderr, "error: %s: unrecognized option '%s'\n", argv[0], arg);          \
      usage(argv[0], 1);                                                               \
   }

#define print_debug_info(vm)                                                           \
   const buzzdebug_entry_t* dbg = buzzdebug_info_get_fromoffset(dbg_buf, &vm->oldpc);  \
   if(dbg != NULL) {                                                                   \
      fprintf(stderr, "%s: execution terminated abnormally at                          \
               \n\r%s:%" PRIu64 ":%" PRIu64 " : %s\n\n",                               \
               bcfname,                                                                \
               (*dbg)->fname,                                                          \
               (*dbg)->line,                                                           \
               (*dbg)->col,                                                            \
               vm->errormsg);                                                          \
   }                                                                                   \
   else {                                                                              \
      fprintf(stderr, "%s: execution terminated abnormally at bytecode offset          \
               \n\r%d: %s\n\n",                                                        \
               bcfname,                                                                \
               vm->oldpc,                                                              \
               vm->errormsg);                                                          \
   }

/* Global variable flag for interrupts */
volatile sig_atomic_t stop_signal;

void interrupt_handler(int signum) {
    stop_signal = 1;
}

/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

/**
 * @brief print Usage
 * 
 * @param path binary path 
 * @param status exit code
 */
void usage(const char* path, int status) {
   fprintf(stderr, "Usage:\n\t%s [--trace] [--server [server_ip:port]] <file.bo> <file.bdb>\n\n", path);
   exit(status);
}

/**
 * @brief print all symbols
 * 
 * @param vm 
 * @return int 
 */
int print(buzzvm_t vm) {
   for(int i = 1; i < buzzdarray_size(vm->lsyms->syms); ++i) {
      buzzvm_lload(vm, i);
      buzzobj_t o = buzzvm_stack_at(vm, 1);
      buzzvm_pop(vm);
      switch(o->o.type) {
         case BUZZTYPE_NIL:
            fprintf(stdout, "[nil]");
            break;
         case BUZZTYPE_INT:
            fprintf(stdout, "%d", o->i.value);
            break;
         case BUZZTYPE_FLOAT:
            fprintf(stdout, "%f", o->f.value);
            break;
         case BUZZTYPE_TABLE:
            fprintf(stdout, "[table with %d elems]", (buzzdict_size(o->t.value)));
            break;
         case BUZZTYPE_CLOSURE:
            if(o->c.value.isnative)
               fprintf(stdout, "[n-closure @%d]", o->c.value.ref);
            else
               fprintf(stdout, "[c-closure @%d]", o->c.value.ref);
            break;
         case BUZZTYPE_STRING:
            fprintf(stdout, "%s", o->s.value.str);
            break;
         case BUZZTYPE_USERDATA:
            fprintf(stdout, "[userdata @%p]", o->u.value);
            break;
         default:
            break;
      }
   }
   fprintf(stdout, "\n");
   return buzzvm_ret0(vm);
}

/**
 * @brief Process incoming messages from server
 * 
 * @param vm 
 * @param server_socket 
 */
void process_in_msgs(buzzvm_t vm, int server_socket) {
   /* first data is robot ID */
   uint16_t robot_id;
   ssize_t bytes_read = read(server_socket, &robot_id, sizeof(uint16_t));
   if(bytes_read == 0) {
      perror("Server disconnected");
      exit(1);
   } else if (bytes_read == -1) {
      return; // nothing to read
   }
   /* Next is the size of payload */
   uint32_t size_of_incoming_data;
   /* read the size of incoming data */
   if((bytes_read = read(server_socket, &size_of_incoming_data, sizeof(uint32_t))) == 0) {
      perror("Server disconnected");
      exit(1);
   } else if (bytes_read == -1) {
      return; // nothing to read
   }
   /* Set Server socket to blocking */
   if (fcntl(server_socket, F_SETFL,  fcntl(server_socket, F_GETFL) & ~(O_NONBLOCK)) < 0) {
      perror("Cannot configure socket to be in blocking mode");
      return;
   }
   printf("size_of_incoming_data: %ld\n", size_of_incoming_data);
   /* If we got any data */
   if (size_of_incoming_data > 0) {
      /* Create an object to store incoming data */
      char data[size_of_incoming_data];
      /* To loop until we read all the info */
      uint32_t total_read = 0;
      char buffer[1024];
      uint32_t to_read_in_loop;
      while (total_read < size_of_incoming_data && !stop_signal) {
         to_read_in_loop = fmin(sizeof(buffer), size_of_incoming_data - total_read);
         printf("to_read_in_loop: %ld, total_read: %ld, size of incoming: %ld\n", to_read_in_loop,  total_read, size_of_incoming_data);
         bytes_read = read(server_socket, buffer, to_read_in_loop);
         if (bytes_read == 0) {
            perror("Server disconnected");
            exit(1);
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
         return; // nothing to read;
      }
      printf("Received new message of [%ld] bytes from Robot: %d\n", total_read, robot_id);
      buzzinmsg_queue_append(vm, robot_id,
                        buzzmsg_payload_frombuffer(buffer, size_of_incoming_data));
      /* Process messages */
      buzzvm_process_inmsgs(vm);
   }
   /* Set Server socket back to non-blocking */
   if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK) < 0) {
      perror("Cannot configure socket to be in non-blocking mode");
      exit(1);
   }
}

/**
 * @brief Process any message to be sent to server
 * 
 * @param vm 
 * @param server_socket 
 */
void process_out_msgs(buzzvm_t vm, int server_socket) {
   /* Process outgoing messages */
   buzzvm_process_outmsgs(vm);
   /* Are there any message in queue */
   if(buzzoutmsg_queue_isempty(vm)) return;
   /* Set Server socket to blocking */
   if (fcntl(server_socket, F_SETFL,  fcntl(server_socket, F_GETFL) & ~(O_NONBLOCK)) < 0) {
      perror("Cannot configure socket to be in blocking mode");
      return;
   }
   /* Loop through buzzoutmsg queue */
   do {
      /* Are there more messages? */
      if(buzzoutmsg_queue_isempty(vm)) break;
      /* Send robot id to server */
      if (write(server_socket, &vm->robot, sizeof(vm->robot)) == -1) {
         perror("Cannot write to server");
         exit(1);
      }
      /* Get first message */
      buzzmsg_payload_t m = buzzoutmsg_queue_first(vm);
      uint32_t payload_size = buzzmsg_payload_size(m);
      /* Send data(payload) size to server */
      if (write(server_socket, &payload_size, sizeof(uint32_t)) == -1) {
         perror("Cannot write to server");
         exit(1);
      }
      /* Send data(payload) to server */
      // printf("Sending %ld bytes from Robot ID: %d\n", buzzmsg_payload_size(m), vm->robot);
      if (write(server_socket, m->data, payload_size) == -1) {
         perror("Cannot write to server");
         exit(1);
      }
      /* Get rid of message */
      buzzoutmsg_queue_next(vm);
      buzzmsg_payload_destroy(&m);
   } while(1);
   /* Set Server socket back to non-blocking */
   if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK) < 0) {
      perror("Cannot configure socket to be in non-blocking mode");
      exit(1);
   }
}

int main(int argc, char** argv) {
   /* The bytecode filename */
   char* bcfname;
   /* The debugging information file name */
   char* dbgfname;
   /* Whether or not to show the assembly information */
   int trace = 0;
   /* Whether or not to run a server with init and loop functions */
   int server = 0;
   /* server socket file descriptor */
   int server_socket;
   /* socket address for the server */
   struct sockaddr_in server_addr;
   /* robot ID of current VM */
   uint16_t robot_id = 1;
   /* Parse command line */

   if(argc < 3 || argc > 6) usage(argv[0], 0);

   for (int i = 1; i < argc - 2; i++) {
      if(strcmp(argv[i], "--trace") == 0) {
         trace = 1;
      } else if(strcmp(argv[i], "--server") == 0) {
         server = 1;
         server_addr.sin_family = AF_INET;
         /* Check if next argument is a TCP/IP address with port, for example 127.0.0.1:8080 */
         if(i + 1 < argc) {
            /* Check if ":" exists in the next argument */
            if(strchr(argv[i + 1], ':') != NULL) {
               /* Get Hostname/IP address */
               char* token = strtok(argv[i + 1], ":");
               server_addr.sin_addr.s_addr = inet_addr(token);
               /* Get port */
               char* port = strtok(NULL, ":");
               /* Parse to uint16_t */
               char* end;
               intmax_t val = strtoimax(port, &end, 10);
               if (val < 0 || val > UINT16_MAX || end == port || *end != '\0') {
                  // Cannot parse as Port, conitnue to next argument
                  continue;
               }
               server_addr.sin_port = htons((uint16_t) val);
               /* Skip the next argument since it is parsed here */
               i++; // skip next
            } else {
               /* If server not specified, give defaults */
               server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
               server_addr.sin_port = htons(8080);
            }
         }
      } else {
         fprintf(stderr, "error: %s: unrecognized option '%s'\n", argv[0], argv[i]);
         usage(argv[0], 1);
      }
   }
   bcfname = argv[argc - 2];
   dbgfname = argv[argc - 1];
   /* If in server mode */
   if(server) {
      printf("Connecting to server at %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
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
      if (connect(server_socket, (struct sockaddr_in *)&server_addr, sizeof(server_addr)) != 0) {
         perror("Cannot connect to server!");
         exit(1);
      } else {
         printf("Connected to the server!\n");
      }
      /* Read Robot ID(First message sent by server is server ID) */
      if(read(server_socket, &robot_id, sizeof(robot_id)) <= 0) {
         perror("Server disconnected / cannot receive Robot ID.");
         exit(1);
      }
      /* Configure the socket to be in non-blocking mode */
      if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK) < 0) {
         perror("Cannot configure socket to be in non-blocking mode");
         exit(1);
      }
   }
   /* Read bytecode and fill in data structure */
   FILE* fd = fopen(bcfname, "rb");
   if(!fd) perror(bcfname);
   fseek(fd, 0, SEEK_END);
   size_t bcode_size = ftell(fd);
   rewind(fd);
   uint8_t* bcode_buf = (uint8_t*)malloc(bcode_size);
   if(fread(bcode_buf, 1, bcode_size, fd) < bcode_size) {
      perror(bcfname);
   }
   fclose(fd);
   /* Read debug information */
   buzzdebug_t dbg_buf = buzzdebug_new();
   if(!buzzdebug_fromfile(dbg_buf, dbgfname)) {
      perror(dbgfname);
   }
   /* Create new VM */
   buzzvm_t vm = buzzvm_new(robot_id);
   /* Set byte code */
   buzzvm_set_bcode(vm, bcode_buf, bcode_size);
   /* Register hook functions */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "log", 1));
   buzzvm_pushcc(vm, buzzvm_function_register(vm, print));
   buzzvm_gstore(vm);
   /* Run byte code */
   do if(trace) buzzdebug_stack_dump(vm, 1, stdout);
   while(buzzvm_step(vm) == BUZZVM_STATE_READY);
   /* Done running, check final state */
   int retval;
   if(vm->state == BUZZVM_STATE_DONE) {
      /* Execution terminated without errors */
      if(trace) buzzdebug_stack_dump(vm, 1, stdout);
      if (!server) { // if not in server mode, print this
         fprintf(stdout, "%s: execution terminated correctly\n\n",
               bcfname);
      }
      retval = 0;
   }
   else {
      /* Execution terminated with errors */
      if(trace) buzzdebug_stack_dump(vm, 1, stdout);
      /* Print debug info to stderr */
      print_debug_info(vm);
      retval = 1;
   }

   if (server) {
      /* Call the Init() function */
      if(buzzvm_function_call(vm, "init", 0) != BUZZVM_STATE_READY) {
         /* Print debug info to stderr */
         print_debug_info(vm);
         retval = 1;
      } else {
         struct timespec startTime, endTime, deltaTime, sleepTime;
         clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);

         /* Infinite Loop breaking with interrupts */
         while(vm->state == BUZZVM_STATE_READY && !stop_signal) {
            /* Check for any incoming messages from server and parse */
            process_in_msgs(vm, server_socket);
            /* If trace is enabled */
            if(trace) buzzdebug_stack_dump(vm, 1, stdout);
            if(buzzvm_function_call(vm, "step", 0) != BUZZVM_STATE_READY) {
               /* Print debug info to stderr */
               print_debug_info(vm);
               retval = 1;
               break;
            }
            /* Remove useless return value from stack */
            buzzvm_pop(vm);
            /* Check for any outgoing messages and send */
            process_out_msgs(vm, server_socket);
            /*====== Loop time management ======*/
            clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
            /* Calculate sleep time */
            timespec_diff(&endTime, &startTime, &deltaTime);
            timespec_diff(&LOOP_TIME_STEP, &deltaTime, &sleepTime);
            /* Sleep */
            nanosleep(&sleepTime, NULL);
            clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);
         }
      }
   }
   /* Destroy VM */
   free(bcode_buf);
   buzzdebug_destroy(&dbg_buf);
   buzzvm_destroy(&vm);
   /* All done */
   return retval;
}
