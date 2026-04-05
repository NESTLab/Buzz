#include <stdio.h>
#include <math.h>
#include <buzz/buzzvm.h>
#include <gsl/gsl_matrix.h>
#include <buzz/buzzutils.h>
#include <buzz/buzzgsl.h>

static int n_pass = 0;
static int n_fail = 0;

#define TEST(NAME, EXPR) do {                            \
   if(EXPR) { printf("[PASS] %s\n", NAME); ++n_pass; } \
   else     { printf("[FAIL] %s\n", NAME); ++n_fail; } \
} while(0)

#define CLOSE(a, b) (fabs((double)(a) - (double)(b)) < 1e-5)

int main(void) {
   buzzvm_t vm = buzzvm_new(0);
   printf("=== Layer 2: buzzgsl ===\n\n");

   /* --- buzzgsl_fromgsl: GSL -> userdata --- */
   gsl_matrix* m = gsl_matrix_alloc(2, 3);
   for(size_t i = 0; i < 2; ++i)
      for(size_t j = 0; j < 3; ++j)
         gsl_matrix_set(m, i, j, (double)(i * 3 + j + 1));
   /* Ownership transferred to Buzz GC */
   buzzobj_t t_obj = buzzgsl_fromgsl(vm, m);
   TEST("fromgsl returns non-null",         t_obj != NULL);
   TEST("fromgsl produces userdata",        buzzobj_isuserdata(t_obj));
   TEST("fromgsl destroy set",              t_obj->u.destroy != NULL);
   TEST("fromgsl clone set",                t_obj->u.clone   != NULL);

   printf("\n");

   /* --- buzzgsl_togsl: userdata -> GSL --- */
   gsl_matrix* m2 = buzzgsl_togsl(vm, t_obj);
   TEST("togsl returns non-null",           m2 != NULL);
   TEST("togsl same pointer",               m2 == m); /* no copy — same ptr */
   TEST("togsl rows",                       m2->size1 == 2);
   TEST("togsl cols",                       m2->size2 == 3);
   TEST("togsl [0][0]",                     CLOSE(gsl_matrix_get(m2, 0, 0), 1.0));
   TEST("togsl [0][2]",                     CLOSE(gsl_matrix_get(m2, 0, 2), 3.0));
   TEST("togsl [1][0]",                     CLOSE(gsl_matrix_get(m2, 1, 0), 4.0));
   TEST("togsl [1][2]",                     CLOSE(gsl_matrix_get(m2, 1, 2), 6.0));

   printf("\n");

   /* --- togsl type error --- */
   buzzvm_pushi(vm, 42);
   buzzobj_t t_int = buzzvm_stack_at(vm, 1);
   buzzvm_pop(vm);
   gsl_matrix* m_bad = buzzgsl_togsl(vm, t_int);
   TEST("togsl type error returns NULL",    m_bad == NULL);
   TEST("togsl type error sets vm error",   vm->state == BUZZVM_STATE_ERROR);
   /* Reset VM state to continue testing */
   vm->state = BUZZVM_STATE_READY;

   printf("\n");

   /* --- clone --- */
   buzzobj_t t_clone = buzzheap_clone(vm, t_obj);
   TEST("clone returns non-null",           t_clone != NULL);
   TEST("clone is userdata",                buzzobj_isuserdata(t_clone));
   gsl_matrix* m_clone = buzzgsl_togsl(vm, t_clone);
   TEST("clone different pointer",          m_clone != m);  /* deep copy */
   TEST("clone [0][0] matches",             CLOSE(gsl_matrix_get(m_clone, 0, 0), 1.0));
   TEST("clone [1][2] matches",             CLOSE(gsl_matrix_get(m_clone, 1, 2), 6.0));
   TEST("clone destroy set",                t_clone->u.destroy != NULL);
   TEST("clone clone set",                  t_clone->u.clone   != NULL);
   /* Mutate original, verify clone is independent */
   gsl_matrix_set(m, 0, 0, 99.0);
   TEST("clone is independent of original", CLOSE(gsl_matrix_get(m_clone, 0, 0), 1.0));

   printf("\n--- %d passed, %d failed ---\n", n_pass, n_fail);
   buzzvm_destroy(&vm);
   return n_fail > 0 ? 1 : 0;
}
