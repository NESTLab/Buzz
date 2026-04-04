#include "buzzutils.h"

/****************************************/
/****************************************/

float buzzobj_tofloat(buzzvm_t  t_vm,
                      buzzobj_t t_obj) {
   if(buzzobj_isfloat(t_obj)) return buzzobj_getfloat(t_obj);
   if(buzzobj_isint(t_obj))   return (float)buzzobj_getint(t_obj);
   buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                   "expected number, got %s",
                   buzztype_desc[t_obj->o.type]);
   return 0.0f;
}

int32_t buzzobj_toint(buzzvm_t  t_vm,
                      buzzobj_t t_obj) {
   if(buzzobj_isint(t_obj))   return buzzobj_getint(t_obj);
   if(buzzobj_isfloat(t_obj)) return (int32_t)buzzobj_getfloat(t_obj);
   buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                   "expected number, got %s",
                   buzztype_desc[t_obj->o.type]);
   return 0;
}

const char* buzzobj_tostr(buzzvm_t  t_vm,
                          buzzobj_t t_obj) {
   if(buzzobj_isstring(t_obj)) return buzzobj_getstring(t_obj);
   buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                   "expected string, got %s",
                   buzztype_desc[t_obj->o.type]);
   return 0;
}

/****************************************/
/****************************************/

#define buzztable_get(KEYTYPE_SIG,KEYTYPE_ARG)                        \
   buzzobj_t buzztable_ ## KEYTYPE_SIG ## get(buzzvm_t    t_vm,       \
                                              buzzobj_t   t_table,    \
                                              KEYTYPE_ARG key) {      \
      buzzvm_push(t_vm, t_table);                                     \
      buzzvm_push ## KEYTYPE_SIG (t_vm, key);                         \
      buzzvm_tget(t_vm);                                              \
      buzzobj_t tVal = buzzvm_stack_at(t_vm, 1);                      \
      buzzvm_pop(t_vm);                                               \
      return (buzzobj_isnil(tVal)) ? NULL : tVal;                     \
   }

buzztable_get(i, int32_t);
buzztable_get(f, float);

buzzobj_t buzztable_sget(buzzvm_t    t_vm,
                         buzzobj_t   t_table,
                         const char* str_key) {
   buzzvm_push(t_vm, t_table);
   buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_key, 1));
   buzzvm_tget(t_vm);
   buzzobj_t tVal = buzzvm_stack_at(t_vm, 1);
   buzzvm_pop(t_vm);
   return (buzzobj_isnil(tVal)) ? NULL : tVal;
}

/****************************************/
/****************************************/

#define buzztable_get_type(RET_SIG, RET_CONV, KEY_SIG, KEY_TYPE, KEY_FMT) \
   RET_SIG buzztable_ ## KEY_SIG ## get_ ## RET_CONV(buzzvm_t  t_vm,      \
                                                     buzzobj_t t_table,   \
                                                     KEY_TYPE   key) {  \
      buzzobj_t tVal = buzztable_ ## KEY_SIG ## get(t_vm, t_table, key);  \
      if(!tVal) {                                                         \
         buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,                         \
                         "key '%" #KEY_FMT "' not found in table", key);  \
         return 0;                                                        \
      }                                                                   \
      return buzzobj_to ## RET_CONV(t_vm, tVal);                          \
   }

buzztable_get_type(int32_t,     int,   i, int32_t, PRId32);
buzztable_get_type(float,       float, i, int32_t, PRId32);
buzztable_get_type(const char*, str,   i, int32_t, PRId32);
buzztable_get_type(int32_t,     int,   f, float, f);
buzztable_get_type(float,       float, f, float, f);
buzztable_get_type(const char*, str,   f, float, f);
buzztable_get_type(int32_t,     int,   s, const char*, s);
buzztable_get_type(float,       float, s, const char*, s);
buzztable_get_type(const char*, str,   s, const char*, s);

/****************************************/
/****************************************/

void buzztable_iset(buzzvm_t  t_vm,
                    buzzobj_t t_table,
                    int32_t   n_key,
                    buzzobj_t t_val) {
   buzzvm_push(t_vm, t_table);
   buzzvm_pushi(t_vm, n_key);
   buzzvm_push(t_vm, t_val);
   buzzvm_tput(t_vm);
}

void buzztable_fset(buzzvm_t  t_vm,
                    buzzobj_t t_table,
                    float     f_key,
                    buzzobj_t t_val) {
   buzzvm_push(t_vm, t_table);
   buzzvm_pushf(t_vm, f_key);
   buzzvm_push(t_vm, t_val);
   buzzvm_tput(t_vm);
}

void buzztable_sset(buzzvm_t    t_vm,
                    buzzobj_t   t_table,
                    const char* str_key,
                    buzzobj_t   t_val) {
   buzzvm_push(t_vm, t_table);
   buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_key, 1));
   buzzvm_push(t_vm, t_val);
   buzzvm_tput(t_vm);
}

#define buzztable_set_type(KEY_SIG, KEY_TYPE, VAL_SIG1, VAL_SIG2, VAL_TYPE) \
   void buzztable_ ## KEY_SIG ## set_ ## VAL_SIG1(buzzvm_t  t_vm,           \
                                                 buzzobj_t t_table,         \
                                                 KEY_TYPE  key,             \
                                                 VAL_TYPE  val) {           \
      buzzvm_push(t_vm, t_table);                                           \
      buzzvm_push ## KEY_SIG(t_vm, key);                                    \
      buzzvm_push ## VAL_SIG2(t_vm, val);                                   \
      buzzvm_tput(t_vm);                                                    \
   }

buzztable_set_type(i, int32_t, int,   i, int32_t);
buzztable_set_type(i, int32_t, float, f, float);
buzztable_set_type(f, float,   int,   i, int32_t);
buzztable_set_type(f, float,   float, f, float);

#define buzztable_set_str(KEY_SIG, KEY_TYPE)                            \
   void buzztable_ ## KEY_SIG ## set_ ## str(buzzvm_t  t_vm,            \
                                             buzzobj_t t_table,         \
                                             KEY_TYPE  key,             \
                                             const char* str_val) {     \
      buzzvm_push(t_vm, t_table);                                       \
      buzzvm_push ## KEY_SIG(t_vm, key);                                \
      buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_val, 1));     \
      buzzvm_tput(t_vm);                                                \
   }

buzztable_set_str(i, int32_t);
buzztable_set_str(f, float);

#define buzztable_sset_type(VAL_SIG1, VAL_SIG2, VAL_TYPE)               \
   void buzztable_sset_ ## VAL_SIG1(buzzvm_t    t_vm,                   \
                                    buzzobj_t   t_table,                \
                                    const char* str_key,                \
                                    VAL_TYPE    val) {                  \
      buzzvm_push(t_vm, t_table);                                       \
      buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_key, 1));     \
      buzzvm_push ## VAL_SIG2(t_vm, val);                               \
      buzzvm_tput(t_vm);                                                \
   }

buzztable_sset_type(int,   i, int32_t);
buzztable_sset_type(float, f, float);

void buzztable_sset_str(buzzvm_t    t_vm,
                        buzzobj_t   t_table,
                        const char* str_key,
                        const char* str_val) {
   buzzvm_push(t_vm, t_table);
   buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_key, 1));
   buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_val, 1));
   buzzvm_tput(t_vm);
}

/****************************************/
/****************************************/

struct buzztable_foreach_params {
   buzzvm_t                vm;
   buzztable_foreach_entry fn;
   void*                   params;
};

void buzztable_foreach_entry_trampoline(const void* pt_key,
                                        void* pt_val,
                                        void* pt_params) {
   struct buzztable_foreach_params* p = pt_params;
   p->fn(p->vm, *(buzzobj_t*)pt_key, *(buzzobj_t*)pt_val, p->params);
}

void buzztable_foreach(buzzvm_t                t_vm,
                       buzzobj_t               t_table,
                       buzztable_foreach_entry t_fn,
                       void*                   pt_params) {
   struct buzztable_foreach_params p = {
      .vm = t_vm,
      .fn = t_fn,
      .params = pt_params
   };
   buzzdict_foreach(t_table->t.value, buzztable_foreach_entry_trampoline, &p);
}

/****************************************/
/****************************************/

buzzobj_t buzzglobal_get(buzzvm_t    t_vm,
                         const char* str_var) {
   buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_var, 1));
   buzzvm_gload(t_vm);
   buzzobj_t tVal = buzzvm_stack_at(t_vm, 1);
   buzzvm_pop(t_vm);
   return (buzzobj_isnil(tVal)) ? NULL : tVal;
}

void buzzglobal_set(buzzvm_t    t_vm,
                    const char* str_var,
                    buzzobj_t   t_obj) {
   buzzvm_pushs(t_vm, buzzvm_string_register(t_vm, str_var, 1));
   buzzvm_push(t_vm, t_obj);
   buzzvm_gstore(t_vm);
}

/****************************************/
/****************************************/
