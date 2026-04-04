#ifndef BUZZUTILS_H
#define BUZZUTILS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <buzz/buzzvm.h>

#ifdef __cplusplus
extern "C" {
#endif

   /****************************************/
   /* Type coercion                        */
   /****************************************/

   /**
    * Extracts an int from a buzzobj_t, accepting both BUZZTYPE_INT and
    * BUZZTYPE_FLOAT (truncates).  Sets a VM type error and returns 0 on mismatch.
    */
   int32_t buzzobj_toint(buzzvm_t  t_vm,
                         buzzobj_t t_obj);

   /**
    * Extracts a float from a buzzobj_t, accepting both BUZZTYPE_INT and
    * BUZZTYPE_FLOAT.  Sets a VM type error and returns 0.0 on mismatch.
    */
   float buzzobj_tofloat(buzzvm_t  t_vm,
                         buzzobj_t t_obj);

   /**
    * Extracts a string from a buzzobj_t.
    * Sets a VM type error and returns 0 on mismatch.
    */
   const char* buzzobj_tostr(buzzvm_t  t_vm,
                             buzzobj_t t_obj);

   /****************************************/
   /* Obj-based table read/write           */
   /****************************************/

   buzzobj_t buzztable_iget(buzzvm_t  t_vm,
                            buzzobj_t t_table,
                            int32_t   n_key);
   buzzobj_t buzztable_fget(buzzvm_t  t_vm,
                            buzzobj_t t_table,
                            float     f_key);
   buzzobj_t buzztable_sget(buzzvm_t    t_vm,
                            buzzobj_t   t_table,
                            const char* str_key);

   int32_t buzztable_iget_int(buzzvm_t  t_vm,
                              buzzobj_t t_table,
                              int32_t   n_key);

   float buzztable_iget_float(buzzvm_t  t_vm,
                              buzzobj_t t_table,
                              int32_t   n_key);

   const char* buzztable_iget_str(buzzvm_t  t_vm,
                                  buzzobj_t t_table,
                                  int32_t   n_key);

   int32_t buzztable_fget_int(buzzvm_t  t_vm,
                              buzzobj_t t_table,
                              float     f_key);

   float buzztable_fget_float(buzzvm_t  t_vm,
                              buzzobj_t t_table,
                              float     f_key);

   const char* buzztable_fget_str(buzzvm_t  t_vm,
                                  buzzobj_t t_table,
                                  float     f_key);

   int32_t buzztable_sget_int(buzzvm_t    t_vm,
                              buzzobj_t   t_table,
                              const char* str_key);

   float buzztable_sget_float(buzzvm_t    t_vm,
                              buzzobj_t   t_table,
                              const char* str_key);

   const char* buzztable_sget_str(buzzvm_t    t_vm,
                                  buzzobj_t   t_table,
                                  const char* str_key);

   void buzztable_iset(buzzvm_t  t_vm,
                       buzzobj_t t_table,
                       int32_t   n_key,
                       buzzobj_t t_val);

   void buzztable_fset(buzzvm_t  t_vm,
                       buzzobj_t t_table,
                       float     f_key,
                       buzzobj_t t_val);
   
   void buzztable_sset(buzzvm_t    t_vm,
                       buzzobj_t   t_table,
                       const char* str_key,
                       buzzobj_t   t_val);
   
   /****************************************/
   /* Table iteration                      */
   /****************************************/

   typedef void (*buzztable_foreach_entry)(buzzvm_t  vm,
                                           buzzobj_t key,
                                           buzzobj_t val,
                                           void*     data);

   /**
    * Iterates over all entries in t_table, calling fn(vm, key, val, data)
    * for each entry.  Order is unspecified (hash map semantics).
    *
    * NOTE: do not push/pop the stack inside fn — BuzzTableGet/Set are safe,
    * raw buzzvm_push/pop are not.
    */
   void buzztable_foreach(buzzvm_t                t_vm,
                          buzzobj_t               t_table,
                          buzztable_foreach_entry t_fn,
                          void*                   pt_data);

   /****************************************/
   /* Global variable access               */
   /****************************************/

   /**
    * Loads a global variable by name.  Returns NULL if not found.
    */
   buzzobj_t buzzglobal_get(buzzvm_t    t_vm,
                            const char* str_var);

   /**
    * Stores obj as a global variable under str_var.
    */
   void buzzglobal_set(buzzvm_t    t_vm,
                       const char* str_var,
                       buzzobj_t   t_obj);

#ifdef __cplusplus
}
#endif

#endif // BUZZUTILS_H
