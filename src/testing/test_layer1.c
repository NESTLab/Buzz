#include <stdio.h>
#include <string.h>
#include <math.h>
#include <buzz/buzzvm.h>
#include <buzz/buzzutils.h>

static int n_pass = 0;
static int n_fail = 0;

#define TEST(NAME, EXPR) do {                            \
   if(EXPR) { printf("[PASS] %s\n", NAME); ++n_pass; } \
   else     { printf("[FAIL] %s\n", NAME); ++n_fail; } \
} while(0)

#define CLOSE(a, b) (fabsf((float)(a) - (float)(b)) < 1e-5f)

/****************************************/
/* foreach accumulator                  */
/****************************************/

struct sum_state { float sum; int count; };

static void sum_floats(buzzvm_t vm, buzzobj_t key, buzzobj_t val, void* data) {
   struct sum_state* s = (struct sum_state*)data;
   if(buzzobj_isfloat(val)) s->sum += buzzobj_getfloat(val);
   if(buzzobj_isint(val))   s->sum += (float)buzzobj_getint(val);
   ++s->count;
}

/****************************************/

int main(void) {
   buzzvm_t vm = buzzvm_new(0);
   printf("=== Layer 1: buzzutils ===\n\n");

   /* --- Type coercion --- */
   buzzvm_pushi(vm, 7);
   buzzobj_t t_int = buzzvm_stack_at(vm, 1); buzzvm_pop(vm);
   TEST("toint from int",    buzzobj_toint(vm, t_int) == 7);
   TEST("tofloat from int",  CLOSE(buzzobj_tofloat(vm, t_int), 7.0f));

   buzzvm_pushf(vm, 3.9f);
   buzzobj_t t_float = buzzvm_stack_at(vm, 1); buzzvm_pop(vm);
   TEST("tofloat from float", CLOSE(buzzobj_tofloat(vm, t_float), 3.9f));
   TEST("toint from float",   buzzobj_toint(vm, t_float) == 3); /* truncates */

   buzzvm_pushs(vm, buzzvm_string_register(vm, "hello", 1));
   buzzobj_t t_str = buzzvm_stack_at(vm, 1); buzzvm_pop(vm);
   TEST("tostr from string",  strcmp(buzzobj_tostr(vm, t_str), "hello") == 0);

   printf("\n");

   /* --- sset / sget --- */
   buzzobj_t t = buzzheap_newobj(vm, BUZZTYPE_TABLE);
   buzztable_sset_int(vm, t, "r", 3);
   buzztable_sset_int(vm, t, "c", 4);
   buzztable_sset_float(vm, t, "x", 1.5f);
   TEST("sset_int / sget_int r",   buzztable_sget_int(vm, t, "r") == 3);
   TEST("sset_int / sget_int c",   buzztable_sget_int(vm, t, "c") == 4);
   TEST("sset_float / sget_float", CLOSE(buzztable_sget_float(vm, t, "x"), 1.5f));
   TEST("sget missing key",        buzztable_sget(vm, t, "nope") == NULL);

   printf("\n");

   /* --- iset / iget --- */
   buzzobj_t d = buzzheap_newobj(vm, BUZZTYPE_TABLE);
   buzztable_iset_float(vm, d, 0, 1.0f);
   buzztable_iset_float(vm, d, 1, 2.0f);
   buzztable_iset_float(vm, d, 2, 3.0f);
   TEST("iset_float / iget_float [0]", CLOSE(buzztable_iget_float(vm, d, 0), 1.0f));
   TEST("iset_float / iget_float [1]", CLOSE(buzztable_iget_float(vm, d, 1), 2.0f));
   TEST("iset_float / iget_float [2]", CLOSE(buzztable_iget_float(vm, d, 2), 3.0f));
   TEST("iget missing key",            buzztable_iget(vm, d, 99) == NULL);

   printf("\n");

   /* --- foreach --- */
   struct sum_state s = { .sum = 0.0f, .count = 0 };
   buzztable_foreach(vm, d, sum_floats, &s);
   TEST("foreach visited all entries", s.count == 3);
   TEST("foreach sum correct",         CLOSE(s.sum, 6.0f));

   printf("\n");

   /* --- global get / set --- */
   buzzobj_t g = buzzheap_newobj(vm, BUZZTYPE_TABLE);
   buzztable_sset_int(vm, g, "v", 42);
   buzzglobal_set(vm, "my_global", g);
   buzzobj_t gr = buzzglobal_get(vm, "my_global");
   TEST("global_set / global_get",       gr != NULL);
   TEST("global_get value intact",       buzztable_sget_int(vm, gr, "v") == 42);
   TEST("global_get missing",            buzzglobal_get(vm, "nope") == NULL);

   printf("\n--- %d passed, %d failed ---\n", n_pass, n_fail);
   buzzvm_destroy(&vm);
   return n_fail > 0 ? 1 : 0;
}
