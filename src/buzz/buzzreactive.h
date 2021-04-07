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
      buzzdarray_t expressions;     // array of expressions to calculate value
      buzzdarray_t fptrlist;        // array of closures to call on change
      buzzdarray_t dependentlist;   // array of next reactive
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
    * Buzz C closure to create a new reactive object.
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern int buzzreactive_create(struct buzzvm_s* vm);

   /*
    * Buzz function to get a free ID for creating a new reactive variable
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern uint16_t buzzreactive_get_new_rid(struct buzzvm_s* vm);
#ifdef __cplusplus
}
#endif

#endif
