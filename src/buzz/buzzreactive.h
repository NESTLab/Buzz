#ifndef BUZZREACTIVE_H
#define BUZZREACTIVE_H

#include <buzz/buzzvm.h>
#include <buzz/buzztype.h>
#include <buzz/buzzdarray.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

   struct buzzreactive_s {
      uint16_t reactive_id;         // Reactive ID
      buzzdarray_t expressions;     // array of expressions to calculate value
      buzzdarray_t fptrlist;        // array of closures to call on change
      buzzdarray_t dependents;      // array of next reactive
   };
   typedef struct buzzreactive_s* buzzreactive_t;

   /*
    * Data for reactive expressions
    */      
   struct buzzreactive_expr_s {
      /* Operator */
      char optr;
      buzzobj_t op1;
      buzzobj_t op2;
   };
   typedef struct buzzreactive_expr_s buzzreactive_expr_t;

   /*
    * Forward declaration of the Buzz VM.
    */
   struct buzzvm_s;

   /*
    * Registers the reactive functions in the vm.
    * @param vm The state of the 
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern int buzzreactive_register(struct buzzvm_s* vm);

   /*
    * Buzz C closure to create a new reactive variable.
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern int buzzreactive_create(struct buzzvm_s* vm);

   /*
    * Buzz function to create a reactive object
    * @param reactive_id id for the new reactive object
    * @return buzzreactive_t 
    */
   buzzreactive_t buzzreactive_create_obj(uint16_t reactive_id);

   /*
    * Buzz function to get a free ID for creating a new reactive variable
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern uint16_t buzzreactive_get_new_rid(struct buzzvm_s* vm);

   /*
    * destroys the specified reactives dictionary
    * @param reactives pointer
    */
   void buzzreactive_destroy(buzzdict_t* reactives);

#ifdef __cplusplus
}
#endif

#endif
