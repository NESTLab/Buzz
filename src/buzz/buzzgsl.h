#ifndef BUZZGSL_H
#define BUZZGSL_H

/**
 * Buzz-GSL bridge.
 *
 * Buzz matrix representation:
 *   {.r = <rows>, .c = <cols>, .d = {0=v00, 1=v01, ..., r*c-1=v(r-1)(c-1)}}
 * Elements stored in row-major order.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <buzz/buzzutils.h>
#include <gsl/gsl_matrix.h>

#ifdef __cplusplus
extern "C" {
#endif

   /**
    * Unpacks a Buzz matrix into a freshly allocated GSL matrix.
    */
   gsl_matrix* buzzgsl_togsl(buzzvm_t  t_vm,
                             buzzobj_t t_obj);

   /**
    * Packs a GSL matrix into a freshly allocated Buzz matrix.
    */
   buzzobj_t buzzgsl_fromgsl(buzzvm_t          t_vm,
                             const gsl_matrix* mat);

#ifdef __cplusplus
}
#endif

#endif // BUZZGSL_H
