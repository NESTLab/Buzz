#ifndef BUZZLINALG_H
#define BUZZLINALG_H

#include <buzz/buzzvm.h>

/**
 * Buzz closures for linear algebra.
 * Registers a "mat" table with the following functions:
 *
 *   mat.new(r, c)       -> mat       zeroed r×c matrix
 *   mat.eye(n)          -> mat       n×n identity
 *   mat.get(m, i, j)    -> float     element access
 *   mat.set(m, i, j, v)              element write (in place)
 *   mat.add(A, B)       -> mat
 *   mat.sub(A, B)       -> mat
 *   mat.scale(A, s)     -> mat
 *   mat.mul(A, B)       -> mat
 *   mat.t(A)            -> mat       transpose
 *   mat.inv(A)          -> mat       inverse (LU)
 *   mat.chol(A)         -> mat       Cholesky lower factor
 */

#ifdef __cplusplus
extern "C" {
#endif

   buzzvm_state buzzlinalg_register(buzzvm_t vm);

#ifdef __cplusplus
}
#endif

#endif /* BUZZLINALG_H */
