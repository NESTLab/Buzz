#include "buzzreactive.h"
#include "buzzvm.h"
#include "buzztype.h"
#include "buzzdarray.h"
#include <stdlib.h>
#include <stdio.h>


#define function_register(FNAME)                                           \
   buzzvm_dup(vm);                                                         \
   buzzvm_pushs(vm, buzzvm_string_register(vm, #FNAME, 1));                \
   buzzvm_pushcc(vm, buzzvm_function_register(vm, buzzreactive_ ## FNAME));\
   buzzvm_tput(vm);


/****************************************/
/****************************************/

int buzzreactive_register(struct buzzvm_s* vm) {
   /* Push 'reactive' table */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "reactive", 1));
   buzzvm_pusht(vm);
   /* Add 'create' function */
   function_register(create);
   /* Register the 'reactive' table */
   buzzvm_gstore(vm);
   return vm->state;
}

/****************************************/
/****************************************/

int buzzreactive_create(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);

   buzzobj_t o = buzzvm_stack_at(vm, 1);
   /* Set the variable as reactive */
   o->o.reactive_id = buzzreactive_get_new_rid(vm);
   if(o->o.reactive_id == UINT16_MAX) {
      vm->state    = BUZZVM_STATE_ERROR;
      vm->error    = BUZZVM_ERROR_RID_LIMIT;
      vm->errormsg = "Cannot create more reactive variables\n";
      buzzvm_pushnil(vm);
      return buzzvm_ret1(vm);
   }
   buzzreactive_t new_reactive = buzzreactive_create_obj(o->o.reactive_id);
   buzzdict_set(vm->reactives, &(o->o.reactive_id), &new_reactive);
   return buzzvm_ret1(vm);
}

/****************************************/
/****************************************/

buzzreactive_t buzzreactive_create_obj(uint16_t reactive_id) {
   /* Create a new reactive obj. calloc() zeroes everything. */
   buzzreactive_t ret   = (buzzreactive_t) calloc(1, sizeof(struct buzzreactive_s));
   ret->reactive_id     = reactive_id;
   ret->dependents      = buzzdarray_new(1, sizeof(uint16_t), NULL);
   ret->fptrlist        = buzzdarray_new(1, sizeof(buzzobj_t), NULL);
   ret->expressions     = buzzdarray_new(1, sizeof(buzzreactive_expr_t), NULL); 
   return ret;
}

/****************************************/
/****************************************/

uint16_t buzzreactive_get_new_rid(buzzvm_t vm) {
   // start reactive_id with 1
   uint16_t new_rid = buzzdict_size(vm->reactives) + 1;
 
   while(new_rid < UINT16_MAX) {
      if (!buzzdict_get(vm->reactives, &(new_rid), buzzreactive_t)) {
         return new_rid;
      }

      new_rid += 1;
   }
   return UINT16_MAX;
}

/****************************************/
/****************************************/

void buzzreactive_destroy(buzzdict_t* reactives) {
   /* TODO:reactive */
   buzzdict_destroy(reactives);
}

/****************************************/
/****************************************/
