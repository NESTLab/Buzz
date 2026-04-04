#include "buzzgsl.h"

/****************************************/
/****************************************/

gsl_matrix* buzzgsl_toGSL(buzzvm_t  t_vm,
                          buzzobj_t t_mat) {
   /* Read dimensions */
   int32_t r = buzztable_sget_int(t_vm, t_mat, "r");
   if(t_vm->state != BUZZVM_STATE_READY) return NULL;
   int32_t c = buzztable_sget_int(t_vm, t_mat, "c");
   if(t_vm->state != BUZZVM_STATE_READY) return NULL;
   /* Get data subtable */
   buzzobj_t t_data = buzztable_sget(t_vm, t_mat, "d");
   if(!t_data) {
      buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                      "matrix table missing field 'd'");
      return NULL;
   }
   /* Allocate and fill row-major */
   gsl_matrix* m = gsl_matrix_alloc(r, c);
   for(size_t i = 0; i < (size_t)r; ++i)
      for(size_t j = 0; j < (size_t)c; ++j)
         gsl_matrix_set(m, i, j,
                        buzztable_iget_float(t_vm, t_data,
                                             (int32_t)(i * c + j)));
   return m;
}

/****************************************/
/****************************************/

buzzobj_t buzzgsl_fromGSL(buzzvm_t          t_vm,
                          const gsl_matrix* mat) {
   int32_t r = (int32_t)mat->size1;
   int32_t c = (int32_t)mat->size2;
   /* Allocate outer table */
   buzzobj_t t_mat = buzzheap_newobj(t_vm, BUZZTYPE_TABLE);
   buzztable_sset_int(t_vm, t_mat, "r", r);
   buzztable_sset_int(t_vm, t_mat, "c", c);
   /* Allocate data table and fill row-major */
   buzzobj_t t_data = buzzheap_newobj(t_vm, BUZZTYPE_TABLE);
   for(size_t i = 0; i < (size_t)r; ++i)
      for(size_t j = 0; j < (size_t)c; ++j)
         buzztable_iset_float(t_vm, t_data,
                              (int32_t)(i * c + j),
                              (float)gsl_matrix_get(mat, i, j));
   /* Store data table — t_mat roots t_data, safe from GC */
   buzztable_sset(t_vm, t_mat, "d", t_data);
   return t_mat;
}

/****************************************/
/****************************************/
