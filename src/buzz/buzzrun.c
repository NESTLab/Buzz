#include <buzz/buzzasm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/* time duration for each cycle in the loop (usually 0.1 seconds) */
struct timespec LOOP_TIME_STEP = { 0, 0.1 * 1e9}; // 0.1 seconds

#define check_arg(arg)                                                                 \
   if(strcmp(arg, "--trace") == 0) {                                                   \
      trace = 1;                                                                       \
   } else if(strcmp(arg, "--loop") == 0) {                                             \
      loop = 1;                                                                        \
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

void usage(const char* path, int status) {
   fprintf(stderr, "Usage:\n\t%s [--trace] [--loop] <file.bo> <file.bdb>\n\n", path);
   exit(status);
}

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

int main(int argc, char** argv) {
   /* The bytecode filename */
   char* bcfname;
   /* The debugging information file name */
   char* dbgfname;
   /* Whether or not to show the assembly information */
   int trace = 0;
   /* Whether or not to run init and loop functions */
   int loop = 0;
   /* Parse command line */
   if(argc < 3 || argc > 5) usage(argv[0], 0);
   if(argc == 3) {
      bcfname = argv[1];
      dbgfname = argv[2];
   } else {
      bcfname = argv[argc - 2];
      dbgfname = argv[argc - 1];
      check_arg(argv[1]);
      if(argc == 5) {
         check_arg(argv[2]);
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
   buzzvm_t vm = buzzvm_new(1);
   /* Set byte code */
   buzzvm_set_bcode(vm, bcode_buf, bcode_size);
   /* Register hook functions */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "log", 1));
   buzzvm_pushcc(vm, buzzvm_function_register(vm, print));
   buzzvm_gstore(vm);

   //TODO: register other functions like controller, print, clear, debug, 

   /* Run byte code */
   do if(trace) buzzdebug_stack_dump(vm, 1, stdout);
   while(buzzvm_step(vm) == BUZZVM_STATE_READY);
   /* Done running, check final state */
   int retval;
   if(vm->state == BUZZVM_STATE_DONE) {
      /* Execution terminated without errors */
      if(trace) buzzdebug_stack_dump(vm, 1, stdout);
      if (!loop) { // if not in loop mode, print this
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

   if (loop) {
      /* Call the Init() function */
      if(buzzvm_function_call(vm, "init", 0) != BUZZVM_STATE_READY) {
         /* Print debug info to stderr */
         print_debug_info(vm);
         retval = 1;
      } else {
         struct timespec startTime, endTime, deltaTime, sleepTime;
         clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);

         /* Infinite Loop */
         while(vm->state == BUZZVM_STATE_READY) {
            // ProcessInMsgs();
            if(trace) buzzdebug_stack_dump(vm, 1, stdout);
            if(buzzvm_function_call(vm, "step", 0) != BUZZVM_STATE_READY) {
               /* Print debug info to stderr */
               print_debug_info(vm);
               retval = 1;
               break;
            }
            /* Remove useless return value from stack */
            buzzvm_pop(vm);
            // ProcessOutMsgs();

            /* Loop time management */
            clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
            /* Calculate sleep time */
            timespec_diff(&endTime, &startTime, &deltaTime);
            timespec_diff(&LOOP_TIME_STEP, &deltaTime, &sleepTime);
            /* Sleep */
            // printf("Sleeps for: %ld.%.9ld\n", sleepTime.tv_sec, sleepTime.tv_nsec);
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
