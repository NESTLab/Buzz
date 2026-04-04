#include "buzzutils.h"

/****************************************/
/****************************************/

float buzzobj_tofloat(buzzvm_t  t_vm,
                      buzzobj_t t_obj) {
   if(t_obj->o.type == BUZZTYPE_FLOAT) return buzzobj_getint(t_obj);
   if(t_obj->o.type == BUZZTYPE_INT)   return (float)buzzobj_getint(t_obj);
   buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                   "expected number, got %s",
                   buzztype_desc[t_obj->o.type]);
   return 0.0f;
}

int32_t buzzobj_toint(buzzvm_t  t_vm,
                      buzzobj_t t_obj) {
   if(t_obj->o.type == BUZZTYPE_INT)   return buzzobj_getint(t_obj);
   if(t_obj->o.type == BUZZTYPE_FLOAT) return (int32_t)buzzobj_getfloat(t_obj);
   buzzvm_seterror(t_vm, BUZZVM_ERROR_TYPE,
                   "expected number, got %s",
                   buzztype_desc[t_obj->o.type]);
   return 0.0f;
}

const char* buzzobj_tostr(buzzvm_t  t_vm,
                          buzzobj_t t_obj) {
   if(t_obj->o.type == BUZZTYPE_STRING) return buzzobj_getstring(t_obj);
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
      return (tVal->o.type != BUZZTYPE_NIL) ? tVal : NULL;            \
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
   return (tVal->o.type != BUZZTYPE_NIL) ? tVal : NULL;
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
   p->fn(p->vm, (buzzobj_t)pt_key, (buzzobj_t)pt_val, p->params);
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
