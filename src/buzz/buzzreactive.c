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

#define binary_op_arith(op1, oper, op2, res)                               \
   switch (oper) {                                                         \
      case '+':                                                            \
         res = op1 + op2;                                                  \
         break;                                                            \
      case '-':                                                            \
         res = op1 - op2;                                                  \
         break;                                                            \
      case '*':                                                            \
         res = op1 * op2;                                                  \
         break;                                                            \
      case '/':                                                            \
         res = op1 / op2;                                                  \
         break;                                                            \
   }

#define buzzvm_binary_op_arith(op1, oper, op2, res)                        \
   if((op1)->o.type == BUZZTYPE_INT &&                                     \
      (op2)->o.type == BUZZTYPE_INT) {                                     \
      res = buzzheap_newobj((vm), BUZZTYPE_INT);                           \
      binary_op_arith((op1)->i.value, oper, (op2)->i.value,                \
                                                   (res)->i.value);        \
   }                                                                       \
   else if((op1)->o.type == BUZZTYPE_INT &&                                \
           (op2)->o.type == BUZZTYPE_FLOAT) {                              \
      res = buzzheap_newobj((vm), BUZZTYPE_FLOAT);                         \
      binary_op_arith((op1)->i.value, oper, (op2)->f.value,                \
                                                   (res)->f.value);        \
   }                                                                       \
   else if((op1)->o.type == BUZZTYPE_FLOAT &&                              \
           (op2)->o.type == BUZZTYPE_INT) {                                \
      res = buzzheap_newobj((vm), BUZZTYPE_FLOAT);                         \
      binary_op_arith((op1)->f.value, oper, (op2)->i.value,                \
                                                   (res)->f.value);        \
   }                                                                       \
   else {                                                                  \
      res = buzzheap_newobj((vm), BUZZTYPE_FLOAT);                         \
      binary_op_arith((op1)->f.value, oper, (op2)->f.value,                \
                                                   (res)->f.value);        \
   }

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
   buzzreactive_create_reactive(vm, o);
   if((vm)->state == BUZZVM_STATE_ERROR) {
      buzzvm_pushnil(vm);
      return buzzvm_ret1(vm);
   }
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
   /* start reactive_id with 1 */
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

int buzzreactive_deps_cmp(const void* a, const void* b) {
   uint16_t d1 = *(uint16_t*)a;
   uint16_t d2 = *(uint16_t*)b;
   return d1 - d2;
}

/****************************************/
/****************************************/

buzzreactive_t buzzreactive_create_reactive(buzzvm_t vm, buzzobj_t obj) {
   /* Create a new reactive obj. calloc() zeroes everything. */
   buzzreactive_t ret = (buzzreactive_t) calloc(1, sizeof(struct buzzreactive_s));
   if (obj->o.reactive_id == 0) {
      obj->o.reactive_id = buzzreactive_get_new_rid(vm);
      if(obj->o.reactive_id == UINT16_MAX) {
         vm->state    = BUZZVM_STATE_ERROR;
         vm->error    = BUZZVM_ERROR_RID_LIMIT;
         vm->errormsg = "Cannot create more reactive variables\n";
         return NULL;
      }
   }
   ret->reactive_id     = obj->o.reactive_id;
   ret->value           = obj;
   ret->dependents      = buzzset_new(sizeof(uint16_t),
                                       buzzreactive_deps_cmp,
                                       NULL);
   ret->fptrlist        = buzzdarray_new(1, 
                                       sizeof(buzzobj_t),
                                       NULL);
   /* Add to reactives dictionary in (vm) */
   buzzdict_set(vm->reactives, &(obj->o.reactive_id), &ret);
   return ret;
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

struct deps_recalc_params {
   buzzdarray_t to_recalculate;
   buzzvm_t vm;
   buzzobj_t obj;
};

void buzzreactive_deps_recalc_each(void* data, void* params_) {
   uint16_t dep_id = *(uint16_t*) data;
   struct deps_recalc_params params = *(struct deps_recalc_params*) params_;
   buzzdarray_push(params.to_recalculate, &dep_id);
   buzzreactive_t dep_var = *buzzdict_get(params.vm->reactives, 
                                       &(dep_id), buzzreactive_t);
   /* update operand the value */
   if (dep_var->expr.op1->o.reactive_id == params.obj->o.reactive_id) {
      dep_var->expr.op1 = params.obj;
   }
   if (dep_var->expr.op2->o.reactive_id == params.obj->o.reactive_id) {
      dep_var->expr.op2 = params.obj;
   }
   buzzreactive_recalc_update_val(params.vm, dep_id, params.obj, params.to_recalculate);
}

void buzzreactive_recalc_update_val(buzzvm_t vm, uint16_t reactive_id,
                              buzzobj_t obj, buzzdarray_t to_recalculate) {
   if(reactive_id != 0) {
      buzzreactive_t reactive_var = *buzzdict_get((vm)->reactives, 
                                       &(reactive_id), buzzreactive_t);
      struct deps_recalc_params params = {
         .vm = vm,
         .to_recalculate = to_recalculate,
         .obj = obj
      };
      /* Loop through all its dependents */
      buzzset_foreach(reactive_var->dependents, buzzreactive_deps_recalc_each, &params);
   }
}

/****************************************/
/****************************************/

void buzzreactive_recalculate(buzzvm_t vm, buzzobj_t obj) {
   /* List of all the variables to recalculate */
   buzzdarray_t to_recalculate = buzzdarray_new(1, sizeof(uint16_t), NULL);
   /* Recursively update all reactive variables values */
   buzzreactive_recalc_update_val(vm, obj->o.reactive_id, obj, to_recalculate);
   /* re-calculate all values using saved expression */
   for (uint32_t i = 0; i < buzzdarray_size(to_recalculate); ++i) {   
      uint16_t dep_id = buzzdarray_get(to_recalculate, i, uint16_t);
      buzzreactive_t reactv = *buzzdict_get((vm)->reactives, 
                                       &(dep_id), buzzreactive_t);
      buzzobj_t res;
      buzzvm_binary_op_arith(reactv->expr.op1, reactv->expr.optr, reactv->expr.op2, res);
      res->o.reactive_id = dep_id;
      *reactv->value = *res;
      buzzdarray_foreach(reactv->fptrlist,
                        buzzreactive_fptrlist_callback_tree, vm);
   }
   // TODO: Write a mechanism to find cyclic loops
   buzzdarray_destroy(&to_recalculate);
}

/****************************************/
/****************************************/

void buzzreactive_destroy(const void* key, void* data, void* params) {
   buzzreactive_t reactiv = *(buzzreactive_t*) data;
   if (reactiv->dependents) {
      buzzset_destroy(&reactiv->dependents);
   }
   if (reactiv->fptrlist) {
      buzzdarray_destroy(&reactiv->fptrlist);
   }
   free(reactiv);
}

/****************************************/
/****************************************/

void buzzreactives_destroy(buzzdict_t* reactives) {
   buzzdict_foreach(*reactives, buzzreactive_destroy, NULL);
   buzzdict_destroy(reactives);
}

/****************************************/
/****************************************/
