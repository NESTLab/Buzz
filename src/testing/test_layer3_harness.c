#include <stdio.h>
#include <stdlib.h>
#include <buzz/buzzvm.h>
#include <buzz/buzzdebug.h>
#include <buzz/buzzlinalg.h>

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

int main(int argc, char* argv[]) {
   uint8_t*      bytecode = NULL;
   size_t        bcode_size;
   buzzvm_t      vm;
   buzzdebug_t   dbg;
   FILE*         f;
   if(argc < 3) {
      fprintf(stderr, "Usage: %s <script.bo> <script.bdb>\n", argv[0]);
      return 1;
   }
   /* Read bytecode */
   f = fopen(argv[1], "rb");
   if(!f) { perror(argv[1]); return 1; }
   fseek(f, 0, SEEK_END);
   bcode_size = (size_t)ftell(f);
   rewind(f);
   bytecode = (uint8_t*)malloc(bcode_size);
   fread(bytecode, 1, bcode_size, f);
   fclose(f);
   /* Create VM and load debug info */
   vm  = buzzvm_new(0);
   dbg = buzzdebug_new();
   if(!buzzdebug_fromfile(dbg, argv[2])) {
      fprintf(stderr, "Failed to load debug info: %s\n", argv[2]);
      return 1;
   }
   /* Load and execute global portion of script */
   if(buzzvm_set_bcode(vm, bytecode, bcode_size) != BUZZVM_STATE_READY) {
      fprintf(stderr, "Failed to load bytecode\n");
      return 1;
   }
   /* Register hook functions */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "log", 1));
   buzzvm_pushcc(vm, buzzvm_function_register(vm, print));
   buzzvm_gstore(vm);
   /* Register mat.* closures */
   buzzlinalg_register(vm);
   if(buzzvm_execute_script(vm) != BUZZVM_STATE_DONE) {
      fprintf(stderr, "Script error: %s\n", vm->errormsg);
      return 1;
   }
   /* Call init() then step() once */
   if(buzzvm_function_call(vm, "init", 0) != BUZZVM_STATE_READY) {
      fprintf(stderr, "init() error: %s\n", vm->errormsg);
      buzzdebug_gsyms_dump(vm, stderr);
      buzzdebug_stack_dump(vm, 1, stderr);
      return 1;
   }
   buzzvm_pop(vm);
   if(buzzvm_function_call(vm, "step", 0) != BUZZVM_STATE_READY) {
      fprintf(stderr, "step() error: %s\n", vm->errormsg);
      buzzdebug_gsyms_dump(vm, stderr);
      buzzdebug_stack_dump(vm, 1, stderr);
      return 1;
   }
   buzzvm_pop(vm);
   buzzvm_destroy(&vm);
   buzzdebug_destroy(&dbg);
   free(bytecode);
   return 0;
}
