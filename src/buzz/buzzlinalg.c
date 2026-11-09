#include "buzzlinalg.h"
#include "buzzgsl.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

/****************************************/
/* mat.rows(m) -> int                      */
/****************************************/

static int buzzmat_rows(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* m = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 3));
   if(!m) return vm->state;
   buzzvm_pushi(vm, m->size1);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.cols(m) -> int                      */
/****************************************/

static int buzzmat_cols(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* m = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 3));
   if(!m) return vm->state;
   buzzvm_pushi(vm, m->size2);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.new(r, c) -> mat                 */
/****************************************/

static int buzzmat_new(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   int32_t r = buzzobj_toint(vm, buzzvm_stack_at(vm, 2));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   int32_t c = buzzobj_toint(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   buzzvm_push(vm, buzzgsl_fromgsl(vm, gsl_matrix_calloc(r, c)));
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.zero(r, c) -> mat                */
/****************************************/

static int buzzmat_zero(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   int32_t r = buzzobj_toint(vm, buzzvm_stack_at(vm, 2));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   int32_t c = buzzobj_toint(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   buzzvm_push(vm, buzzgsl_fromgsl(vm, gsl_matrix_calloc(r, c)));
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.all(r, c, x) -> mat              */
/****************************************/

static int buzzmat_all(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 3);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_lload(vm, 3);
   int32_t r = buzzobj_toint(vm, buzzvm_stack_at(vm, 3));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   int32_t c = buzzobj_toint(vm, buzzvm_stack_at(vm, 2));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   float x = buzzobj_tofloat(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   gsl_matrix* m = gsl_matrix_alloc(r, c);
   gsl_matrix_set_all(m, x);
   buzzvm_push(vm, buzzgsl_fromgsl(vm, m));
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.eye(n) -> mat                    */
/****************************************/

static int buzzmat_eye(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);
   int32_t n = buzzobj_toint(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   gsl_matrix* m = gsl_matrix_alloc(n, n);
   gsl_matrix_set_identity(m);
   buzzvm_push(vm, buzzgsl_fromgsl(vm, m));
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.get(m, i, j) -> float            */
/****************************************/

static int buzzmat_get(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 3);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_lload(vm, 3);
   buzzvm_type_assert(vm, 3, BUZZTYPE_USERDATA);
   gsl_matrix* m = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 3));
   if(!m) return vm->state;
   int32_t i = buzzobj_toint(vm, buzzvm_stack_at(vm, 2));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   int32_t j = buzzobj_toint(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   buzzvm_pushf(vm, (float)gsl_matrix_get(m, i, j));
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.set(m, i, j, v)                  */
/****************************************/

static int buzzmat_set(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 4);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_lload(vm, 3);
   buzzvm_lload(vm, 4);
   gsl_matrix* m = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 4));
   if(!m) return vm->state;
   int32_t i = buzzobj_toint(vm, buzzvm_stack_at(vm, 3));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   int32_t j = buzzobj_toint(vm, buzzvm_stack_at(vm, 2));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   float v = buzzobj_tofloat(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   gsl_matrix_set(m, (size_t)i, (size_t)j, (double)v);
   return buzzvm_ret0(vm);
}

/****************************************/
/* mat.add(A, B) -> mat                 */
/****************************************/

static int buzzmat_add(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_type_assert(vm, 2, BUZZTYPE_USERDATA);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 2));
   if(!A) return vm->state;
   gsl_matrix* B = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 1));
   if(!B) return vm->state;            /* no gsl_matrix_free(A) — we don't own it */
   if(A->size1 != B->size1 || A->size2 != B->size2) {
      buzzvm_seterror(vm, BUZZVM_ERROR_TYPE, "mat.add: dimension mismatch");
      return vm->state;
   }
   gsl_matrix* result = gsl_matrix_alloc(A->size1, A->size2);
   gsl_matrix_memcpy(result, A);
   gsl_matrix_add(result, B);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, result);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.sub(A, B) -> mat                 */
/****************************************/

static int buzzmat_sub(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_type_assert(vm, 2, BUZZTYPE_USERDATA);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 2));
   if(!A) return vm->state;
   gsl_matrix* B = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 1));
   if(!B) return vm->state;            /* no gsl_matrix_free(A) — we don't own it */
   if(A->size1 != B->size1 || A->size2 != B->size2) {
      buzzvm_seterror(vm, BUZZVM_ERROR_TYPE, "mat.sub: dimension mismatch");
      return vm->state;
   }
   gsl_matrix* result = gsl_matrix_alloc(A->size1, A->size2);
   gsl_matrix_memcpy(result, A);
   gsl_matrix_sub(result, B);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, result);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.scale(A, s) -> mat               */
/****************************************/

static int buzzmat_scale(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_type_assert(vm, 2, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 2));
   if(!A) return vm->state;
   float s = buzzobj_tofloat(vm, buzzvm_stack_at(vm, 1));
   if(vm->state != BUZZVM_STATE_READY) return vm->state;
   gsl_matrix* result = gsl_matrix_alloc(A->size1, A->size2);
   gsl_matrix_memcpy(result, A);
   gsl_matrix_scale(result, (double)s);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, result);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.mul(A, B) -> mat                 */
/****************************************/

static int buzzmat_mul(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_type_assert(vm, 2, BUZZTYPE_USERDATA);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 2));
   if(!A) return vm->state;
   gsl_matrix* B = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 1));
   if(!B) return vm->state;
   if(A->size2 != B->size1) {
      buzzvm_seterror(vm, BUZZVM_ERROR_TYPE, "mat.mul: dimension mismatch");
      return vm->state;
   }
   gsl_matrix* C = gsl_matrix_calloc(A->size1, B->size2);
   gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, A, B, 0.0, C);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, C);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.t(A) -> mat                      */
/****************************************/

static int buzzmat_t(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 1));
   if(!A) return vm->state;
   gsl_matrix* At = gsl_matrix_alloc(A->size2, A->size1);
   gsl_matrix_transpose_memcpy(At, A);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, At);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.inv(A) -> mat                    */
/****************************************/

static int buzzmat_inv(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 1));
   if(!A) return vm->state;
   if(A->size1 != A->size2) {
      buzzvm_seterror(vm, BUZZVM_ERROR_TYPE, "mat.inv: matrix must be square");
      return vm->state;
   }
   /* Work on a copy — LU decomp overwrites its input */
   gsl_matrix* A_copy = gsl_matrix_alloc(A->size1, A->size2);
   gsl_matrix_memcpy(A_copy, A);
   gsl_permutation* perm = gsl_permutation_alloc(A->size1);
   int signum;
   gsl_linalg_LU_decomp(A_copy, perm, &signum);
   gsl_matrix* inv = gsl_matrix_alloc(A->size1, A->size1);
   gsl_linalg_LU_invert(A_copy, perm, inv);
   gsl_matrix_free(A_copy);
   gsl_permutation_free(perm);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, inv);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* mat.chol(A) -> mat                   */
/* Returns lower triangular factor L    */
/* such that A = L * L^T                */
/****************************************/

static int buzzmat_chol(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_USERDATA);
   gsl_matrix* A = buzzgsl_togsl(vm, buzzvm_stack_at(vm, 1));
   if(!A) return vm->state;
   if(A->size1 != A->size2) {
      buzzvm_seterror(vm, BUZZVM_ERROR_TYPE, "mat.chol: matrix must be square");
      return vm->state;
   }
   /* Work on a copy — Cholesky overwrites its input */
   gsl_matrix* A_copy = gsl_matrix_alloc(A->size1, A->size2);
   gsl_matrix_memcpy(A_copy, A);
   gsl_linalg_cholesky_decomp(A_copy);
   for(size_t i = 0; i < A_copy->size1; ++i)
      for(size_t j = i + 1; j < A_copy->size2; ++j)
         gsl_matrix_set(A_copy, i, j, 0.0);
   buzzobj_t t_result = buzzgsl_fromgsl(vm, A_copy);
   buzzvm_push(vm, t_result);
   return buzzvm_ret1(vm);
}

/****************************************/
/* Registration                         */
/****************************************/

#define function_register(FNAME)                                        \
   buzzvm_dup(vm);                                                      \
   buzzvm_pushs(vm, buzzvm_string_register(vm, #FNAME, 1));             \
   buzzvm_pushcc(vm, buzzvm_function_register(vm, buzzmat_ ## FNAME));  \
   buzzvm_tput(vm);

buzzvm_state buzzlinalg_register(buzzvm_t vm) {
   buzzvm_pushs(vm, buzzvm_string_register(vm, "mat", 1));
   buzzvm_pusht(vm);
   function_register(rows);
   function_register(cols);
   function_register(new);
   function_register(zero);
   function_register(all);
   function_register(eye);
   function_register(get);
   function_register(set);
   function_register(add);
   function_register(sub);
   function_register(scale);
   function_register(mul);
   function_register(t);
   function_register(inv);
   function_register(chol);
   buzzvm_gstore(vm);
   return vm->state;
}
