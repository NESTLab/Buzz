#ifndef BUZZGSL_H
#define BUZZGSL_H

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
