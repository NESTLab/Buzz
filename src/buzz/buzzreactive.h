#ifndef BUZZREACTIVE_H
#define BUZZREACTIVE_H

#include <buzz/buzzvm.h>
#include <buzz/buzztype.h>
#include <buzz/buzzdarray.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern void buzzreactive_recalculate(struct buzzvm_s* vm, buzzobj_t reactv);

    extern uint32_t buzzreactive_get_new_rid(struct buzzvm_s* vm);

#ifdef __cplusplus
}
#endif

#endif
