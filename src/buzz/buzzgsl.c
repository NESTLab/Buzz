#include "buzzgsl.h"
#include "buzztype.h"

/****************************************/
/****************************************/

static void  buzzgsl_matrix_destroy(void* value) {
   gsl_matrix_free((gsl_matrix*)value);
}

static void* buzzgsl_matrix_clone(void* value) {
   const gsl_matrix* src = (const gsl_matrix*)value;
   gsl_matrix* dst = gsl_matrix_alloc(src->size1, src->size2);
   gsl_matrix_memcpy(dst, src);
   return dst;
}

/****************************************/
/****************************************/

gsl_matrix* buzzgsl_togsl(buzzvm_t  t_vm,
                          buzzobj_t t_obj) {
   if(!buzzobj_isuserdata(t_obj)) {
      buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                      "expected a matrix (userdata), got %s",
                      buzztype_desc[t_obj->o.type]);
      return NULL;
   }
   return (gsl_matrix*)t_obj->u.value;
}

/****************************************/
/****************************************/

buzzobj_t buzzgsl_fromgsl(buzzvm_t          t_vm,
                          const gsl_matrix* mat) {
   buzzobj_t o = buzzheap_newobj(t_vm, BUZZTYPE_USERDATA);
   o->u.value = (void*)mat;
   o->u.destroy = buzzgsl_matrix_destroy;
   o->u.clone = buzzgsl_matrix_clone;
   return o;
}

/****************************************/
/****************************************/
