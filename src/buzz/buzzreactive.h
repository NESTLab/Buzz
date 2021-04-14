#ifndef BUZZREACTIVE_H
#define BUZZREACTIVE_H

#include <buzz/buzzvm.h>
#include <buzz/buzzset.h>
#include <buzz/buzztype.h>
#include <buzz/buzzdarray.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

   struct buzzreactive_s {
      uint16_t reactive_id;         // Reactive ID
      buzzobj_t value;              // pointer to buzzobj_u variable
      struct {
         char optr;
         buzzobj_t op1;
         buzzobj_t op2;
      } expr;                       // expression to be used to recalculate value
      buzzdarray_t fptrlist;        // array of closures to call on change
      buzzset_t dependents;         // array of next reactive
   };
   typedef struct buzzreactive_s* buzzreactive_t;

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
    * @param vm The Buzz VM state
    * @param obj the object to create upon
    * @return buzzreactive_t 
    */
   extern buzzreactive_t buzzreactive_create_reactive(buzzvm_t vm, buzzobj_t obj);

   /*
    * Buzz reactive function to recalculate value of a reactive variable.
    * @param vm The Buzz VM state.
    * @param obj buzzobj_t to recalculate(must be a reactive variable)
    * @return The updated VM state.
    */
   extern void buzzreactive_recalculate(struct buzzvm_s* vm, buzzobj_t obj);

   /*
    * destroys the specified reactives dictionary
    * @param reactives pointer
    */
   extern void buzzreactives_destroy(buzzdict_t* reactives);

   void buzzreactive_recalc_update_val(buzzvm_t vm, uint16_t reactive_id,
                              buzzobj_t obj, buzzdarray_t to_recalculate);


#ifdef __cplusplus
}
#endif

#endif
