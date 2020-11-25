#include "buzzreactive.h"
#include "buzzvm.h"
#include "buzztype.h"
#include "buzzdarray.h"
#include <stdlib.h>
#include <stdio.h>


#define rid_get()                                                          \
   buzzvm_lload(vm, 0);                                                    \
   buzzvm_pushs(vm, buzzvm_string_register(vm, "rid", 1));                 \
   buzzvm_tget(vm);                                                        \
   uint16_t rid = buzzvm_stack_at(vm, 1)->i.value;

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
   buzzvm_pushs(vm, buzzvm_string_register(vm, "buzzreactive_on_change", 1));
   buzzvm_pushcc(vm, buzzvm_function_register(vm, buzzreactive_on_change));
   buzzvm_gstore(vm);
   return vm->state;
}

/****************************************/
/****************************************/

int buzzreactive_create(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   /* Get reactive id */
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_INT);
   uint32_t value = buzzvm_stack_at(vm, 1)->i.value;
   buzzvm_pop(vm);
   /* Create a new reactive object */
   buzzobj_t o = buzzheap_newobj(vm, BUZZTYPE_REACTIVE);
   o->r.value.value = value;
   o->r.value.rid = buzzreactive_get_new_rid(vm);
   if(o->r.value.rid == INT32_MAX) {
      vm->state    = BUZZVM_STATE_ERROR;
      vm->error    = BUZZVM_ERROR_RID_LIMIT;
      vm->errormsg = "Cannot create more reactive variables\n";
      buzzvm_pushnil(vm);
      return buzzvm_ret1(vm);      
   }
   buzzdict_set(vm->reactives, &(o->r.value.rid), &o);
   buzzvm_push(vm, o);
   return buzzvm_ret1(vm);
}

/****************************************/
/****************************************/

int buzzreactive_on_change(buzzvm_t vm) {
   /* Reactive and closure expected */
   buzzvm_lnum_assert(vm, 2);
   /* Get Reactive */
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_REACTIVE);
   buzzobj_t t = buzzvm_stack_at(vm, 1);
   /* Get closure */
   buzzvm_lload(vm, 2);
   buzzvm_type_assert(vm, 1, BUZZTYPE_CLOSURE);
   buzzobj_t c = buzzvm_stack_at(vm, 1);
   /* Get reactive from vm->reactives */
   const buzzobj_t* reactv = buzzdict_get(vm->reactives, &(t->r.value.rid), buzzobj_t);
   buzzdarray_push((*reactv)->r.value.fptrlist, &c);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

void buzzreactive_recalculate_tree(uint32_t pos, void* data, void* params) {
   buzzvm_t vm = (buzzvm_t)params;
   const buzzobj_t *reactv_ptr = buzzdict_get(vm->reactives, (uint32_t*)data, buzzobj_t);

   buzzreactive_recalculate(vm, *reactv_ptr);
}

/****************************************/
/****************************************/

void buzzreactive_fptrlist_callback_tree(uint32_t pos, void* data, void* params) {
   buzzvm_t vm = (buzzvm_t)params;
   if(vm->state != BUZZVM_STATE_READY) return;
   buzzvm_push(vm, (*(buzzobj_t*)data));
   /* Call closure */
   vm->state = buzzvm_closure_call(vm, 0); 
}

/****************************************/
/****************************************/

void buzzreactive_recalculate(buzzvm_t vm, buzzobj_t reactv) {
   /* Calculate value by looping through all expressions */
   for (size_t i = 0; i < buzzdarray_size(reactv->r.value.expressions);
                                                                     i++) {
      const buzzvm_react_expr_t *expr =
                     &buzzdarray_get(reactv->r.value.expressions, i,
                     buzzvm_react_expr_t);
      int val1 = 0;
      if(expr->op1->o.type == BUZZTYPE_REACTIVE) {
         val1 = (*buzzdict_get((vm)->reactives,
                     &(expr->op1->r.value.rid), buzzobj_t))->r.value.value;
      } else if (expr->op1->o.type == BUZZTYPE_INT) {
         val1 = expr->op1->i.value;
      }
      int val2 = 0;
      if(expr->op2->o.type == BUZZTYPE_REACTIVE) {
         val2 = (*buzzdict_get((vm)->reactives,
                     &(expr->op2->r.value.rid), buzzobj_t))
                     ->r.value.value;
      } else if (expr->op2->o.type == BUZZTYPE_INT) {
         val2 = expr->op2->i.value;
      }

      switch (expr->optr) {
      case '+':
         reactv->r.value.value = val2 + val1;
         break;
      case '-':
         reactv->r.value.value = val2 - val1;
         break;
      case '*':
         reactv->r.value.value = val2 * val1;
         break;
      case '/':
         reactv->r.value.value = val2 / val1;
         break;
      }
   }

   buzzdarray_foreach(reactv->r.value.fptrlist,
                     buzzreactive_fptrlist_callback_tree, vm);
   buzzdarray_foreach(reactv->r.value.dependentlist,
                     buzzreactive_recalculate_tree, vm);
}

/****************************************/
/****************************************/

uint32_t buzzreactive_get_new_rid(buzzvm_t vm) {
   uint32_t new_rid = buzzdict_size(vm->reactives);
 
   while(new_rid < INT32_MAX) {
      if (!buzzdict_get(vm->reactives, &(new_rid), buzzobj_t)) {
         return new_rid;
      }

      new_rid += 1;
   }
   return INT32_MAX;
}


/****************************************/
/****************************************/
