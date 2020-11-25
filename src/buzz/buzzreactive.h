#ifndef BUZZREACTIVE_H
#define BUZZREACTIVE_H

#include <buzz/buzzvm.h>
#include <buzz/buzztype.h>
#include <buzz/buzzdarray.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


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
    * Buzz C closure to register on_change callbacks
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern int buzzreactive_on_change(struct buzzvm_s* vm);

   /*
    * Buzz reactive function to recalculate value of a reactive variable.
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern void buzzreactive_recalculate(struct buzzvm_s* vm, buzzobj_t reactv);

   /*
    * Buzz function to get a free ID for creating a new reactive variable
    * @param vm The Buzz VM state.
    * @return The updated VM state.
    */
   extern uint32_t buzzreactive_get_new_rid(struct buzzvm_s* vm);

#ifdef __cplusplus
}
#endif

#endif
