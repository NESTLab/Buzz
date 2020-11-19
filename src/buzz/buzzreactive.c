#include "buzzreactive.h"
#include "buzzvm.h"
#include "buzztype.h"
#include "buzzdarray.h"
#include <stdlib.h>
#include <stdio.h>

/****************************************/
/****************************************/

void buzzreactive_recalculate_tree(uint32_t pos, void* data, void* params) {
   buzzvm_t vm = (buzzvm_t)params;
   const buzzobj_t *reactv_ptr = buzzdict_get(vm->reactives, (uint32_t*)data, buzzobj_t);

   buzzreactive_recalculate(vm, *reactv_ptr);
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
