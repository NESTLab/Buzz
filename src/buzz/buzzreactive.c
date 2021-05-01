#include "buzzreactive.h"
#include "buzzvm.h"
#include "buzztype.h"
#include "buzzdarray.h"
#include <stdlib.h>
#include <stdio.h>


#define function_register(FNAME)                                           \
   buzzvm_dup(vm);                                                         \
   buzzvm_pushs(vm, buzzvm_string_register(vm, #FNAME, 1));                \
   buzzvm_pushcc(vm, buzzvm_function_register(vm, buzzreactive_ ## FNAME));\
   buzzvm_tput(vm);


#define buzzvm_binary_op(op1, oper, op2, res)                              \
   switch (oper) {                                                         \
      case '+':                                                            \
         res = op1 + op2;                                                  \
         break;                                                            \
      case '-':                                                            \
         res = op1 - op2;                                                  \
         break;                                                            \
      case '*':                                                            \
         res = op1 * op2;                                                  \
         break;                                                            \
      case '/':                                                            \
         res = op1 / op2;                                                  \
         break;                                                            \
   }

#define buzzvm_binary_op_type(op1, oper, op2, res)                         \
   if((op1)->o.type == BUZZTYPE_INT &&                                     \
      (op2)->o.type == BUZZTYPE_INT) {                                     \
      res = buzzheap_newobj((vm), BUZZTYPE_INT);                           \
      buzzvm_binary_op((op1)->i.value, oper, (op2)->i.value,               \
                                                   (res)->i.value);        \
   }                                                                       \
   else if((op1)->o.type == BUZZTYPE_INT &&                                \
           (op2)->o.type == BUZZTYPE_FLOAT) {                              \
      res = buzzheap_newobj((vm), BUZZTYPE_FLOAT);                         \
      buzzvm_binary_op((op1)->i.value, oper, (op2)->f.value,               \
                                                   (res)->f.value);        \
   }                                                                       \
   else if((op1)->o.type == BUZZTYPE_FLOAT &&                              \
           (op2)->o.type == BUZZTYPE_INT) {                                \
      res = buzzheap_newobj((vm), BUZZTYPE_FLOAT);                         \
      buzzvm_binary_op((op1)->f.value, oper, (op2)->i.value,               \
                                                   (res)->f.value);        \
   }                                                                       \
   else {                                                                  \
      res = buzzheap_newobj((vm), BUZZTYPE_FLOAT);                         \
      buzzvm_binary_op((op1)->f.value, oper, (op2)->f.value,               \
                                                   (res)->f.value);        \
   }

/****************************************/
/****************************************/

int buzzreactive_register(struct buzzvm_s* vm) {
   /* Push 'reactive' table */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "reactive", 1));
   buzzvm_pusht(vm);
   /* Add 'create' function */
   function_register(create);
   /* Register the 'reactive' table */
   buzzvm_gstore(vm);
   return vm->state;
}

/****************************************/
/****************************************/

int buzzreactive_create(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   buzzvm_lload(vm, 1);

   buzzobj_t o = buzzvm_stack_at(vm, 1);
   /* Set the variable as reactive */
   buzzreactive_create_reactive(vm, o);
   if((vm)->state == BUZZVM_STATE_ERROR) {
      buzzvm_pushnil(vm);
      return buzzvm_ret1(vm);
   }
   return buzzvm_ret1(vm);
}

/****************************************/
/****************************************/

uint16_t buzzreactive_get_new_rid(buzzvm_t vm) {
   /* start reactive_id with 1 */
   uint16_t new_rid = buzzdict_size(vm->reactives) + 1;
 
   while(new_rid < UINT16_MAX) {
      if (!buzzdict_get(vm->reactives, &(new_rid), buzzreactive_t)) {
         return new_rid;
      }

      new_rid += 1;
   }
   return UINT16_MAX;
}

/****************************************/
/****************************************/

int buzzreactive_cmp(const void* a, const void* b) {
   uint16_t d1 = *(uint16_t*)a;
   uint16_t d2 = *(uint16_t*)b;
   return d1 - d2;
}

/****************************************/
/****************************************/

buzzreactive_t buzzreactive_create_reactive(buzzvm_t vm, buzzobj_t obj) {
   /* Create a new reactive obj. calloc() zeroes everything. */
   buzzreactive_t ret = (buzzreactive_t) calloc(1, sizeof(struct buzzreactive_s));
   if (obj->o.reactive_id == 0) {
      obj->o.reactive_id = buzzreactive_get_new_rid(vm);
      if(obj->o.reactive_id == UINT16_MAX) {
         vm->state    = BUZZVM_STATE_ERROR;
         vm->error    = BUZZVM_ERROR_RID_LIMIT;
         vm->errormsg = "Cannot create more reactive variables\n";
         return NULL;
      }
   }
   ret->reactive_id     = obj->o.reactive_id;
   ret->value           = obj;
   ret->dependents      = buzzset_new(sizeof(uint16_t),
                                       buzzreactive_cmp,
                                       NULL);
   ret->fptrlist        = buzzdarray_new(1, 
                                       sizeof(buzzobj_t),
                                       NULL);
   /* Add to reactives dictionary in (vm) */
   buzzdict_set(vm->reactives, &(obj->o.reactive_id), &ret);
   return ret;
}

/****************************************/
/****************************************/

typedef struct {
   unsigned int first;
   unsigned int second;
} edge;

typedef struct {
   buzzvm_t vm;
   buzzobj_t origin_obj;
   unsigned int max_reactives;
   edge *edges;
   uint16_t *visited;
   uint16_t cur_index;
} buzz_reactive_recalc_params_t;

typedef struct {
   buzz_reactive_recalc_params_t* params;
   uint16_t current_reactive_id;
} buzz_reactive_params_t;

void spanning_tree_recursive(buzz_reactive_recalc_params_t* params, uint16_t edge);

/****************************************/
/****************************************/

void buzzreactive_deps_recalc_each(void* data, void* params_) {
   uint16_t dep_id = *(uint16_t*) data;
   buzz_reactive_params_t* pars = (buzz_reactive_params_t*) params_;
   buzz_reactive_recalc_params_t* params = pars->params;

   if (!params->visited[dep_id]) {
      buzzreactive_t dep_var = *buzzdict_get(params->vm->reactives, 
                                       &(dep_id), buzzreactive_t);
      /* update operand with the value */
      if (dep_var->expr.op1->o.reactive_id == params->origin_obj->o.reactive_id) {
         dep_var->expr.op1 = params->origin_obj;
      }
      if (dep_var->expr.op2->o.reactive_id == params->origin_obj->o.reactive_id) {
         dep_var->expr.op2 = params->origin_obj;
      }
      edge ed = {
         .first = pars->current_reactive_id,
         .second = dep_id
      };
      params->edges[params->cur_index++] = ed;
      params->visited[dep_id] = 1;
      spanning_tree_recursive(params, dep_id);
   }
}

/****************************************/
/****************************************/

void spanning_tree_recursive(buzz_reactive_recalc_params_t* params, uint16_t reactive_id) {
   buzzreactive_t reactive_var = *buzzdict_get(params->vm->reactives, 
                                       &(reactive_id), buzzreactive_t);
   buzz_reactive_params_t pars = {
      .current_reactive_id = reactive_id,
      .params = params
   };
   buzzset_foreach(reactive_var->dependents, buzzreactive_deps_recalc_each, &pars);
}

/****************************************/
/****************************************/

size_t BFS(uint16_t start_reactive_id, edge* edges, const size_t MAX_SIZE, uint16_t * out) {
   struct queue {
      uint16_t reactive_ids[MAX_SIZE];
      size_t front;
      size_t rear;
   };
   /* Create variables */
   struct queue my_queue;
   my_queue.front = -1;
   my_queue.rear = -1;
   uint16_t *visited = calloc(MAX_SIZE, sizeof(uint16_t));
   size_t out_idx = 0;
   size_t visited_idx = 0;
   
   /* Add first edge to the queue */
   my_queue.front = 0;
   my_queue.rear = 0;
   my_queue.reactive_ids[0] = start_reactive_id;
   out[out_idx++] = start_reactive_id;

   /* Run until queue is not empty */
   while (my_queue.front <= my_queue.rear) {
      /* pop from queue */
      uint16_t val = my_queue.reactive_ids[my_queue.front++];
      /* Add it to visited */
      visited[visited_idx++] = val;
      /* loop through all edges to find ones starting with val */
      for (uint16_t i = 0; i < MAX_SIZE; i++) {
         if(edges[i].first == val) {
            /* if its not in visited */
            int second_is_visited = 0;
            for (size_t j = 0; j < visited_idx; j++) {
               if (visited[j] == edges[i].second) {
                  second_is_visited = 1;
                  break;
               }
            }
            /* If the second was not visited */
            if (!second_is_visited) {
               /* Add to the queue */
               my_queue.reactive_ids[++my_queue.rear] = edges[i].second;
               /* Add to output */
               out[out_idx++] = edges[i].second;
            }
         }
      }
   }
   free(visited);
   return out_idx;
}

/****************************************/
/****************************************/

void buzzreactive_fptrlist_callback_tree(uint32_t pos, void* data, void* params) {
   buzzvm_t vm = (buzzvm_t)params;
   if(vm->state != BUZZVM_STATE_READY) return;
   buzzvm_push(vm, (*(buzzobj_t*)data));
   /* Call closure */
   vm->state = buzzvm_closure_call(vm, 0); 
}

/****************************************/
/****************************************/

void buzzreactive_recalculate(buzzvm_t vm, buzzobj_t obj) {
   /* Recursively update all reactive variables values */
   const unsigned int MAX_REACTIVES_COUNT = buzzdict_size(vm->reactives);
   buzz_reactive_recalc_params_t params = {
      .vm = vm,
      .origin_obj = obj,
      .max_reactives = MAX_REACTIVES_COUNT,
      .edges = malloc((MAX_REACTIVES_COUNT) * sizeof(edge)),
      .visited = calloc(MAX_REACTIVES_COUNT, sizeof(uint16_t)),
      .cur_index = 0
   };
   if (params.visited == NULL || params.edges == NULL) {
      free(params.visited);
      free(params.edges);
      params.edges = NULL;
      return ;            // TODO
   }
   /* Run spanning tree to get only once */
   spanning_tree_recursive(&params, obj->o.reactive_id);
   /* Max size is double the number of edges() */
   const size_t MAX_SIZE = params.cur_index * 2;
   /* run BFS to get the order right */
   uint16_t *reactives_list = calloc(MAX_SIZE, sizeof(uint16_t));
   size_t count = BFS(obj->o.reactive_id, params.edges, MAX_SIZE, reactives_list);
   /* Print */
   for (uint16_t i = 0; i < params.cur_index; i++) {
      printf("[%d] : %d -> %d\n", i, params.edges[i].first, params.edges[i].second);
   }
   printf("OUTPUT (%ld):\n", count);

   printf("Starting with : <reactive_id:%d>\n", obj->o.reactive_id);
   for (size_t i = 1; i < count; i++) {
      printf(" %d ->", reactives_list[i]);
      buzzreactive_t reactv = *buzzdict_get((vm)->reactives, 
                                       &(reactives_list[i]), buzzreactive_t);
      if(reactv) {
         buzzobj_t res;
         buzzvm_binary_op_type(reactv->expr.op1, 
                                       reactv->expr.optr, reactv->expr.op2, res);
         res->o.reactive_id = reactives_list[i];
         *reactv->value = *res;
         buzzdarray_foreach(reactv->fptrlist,
                                       buzzreactive_fptrlist_callback_tree, vm);
      }
   }
   printf("\n");

   free(params.visited);
   free(reactives_list);
   free(params.edges);
}

/****************************************/
/****************************************/

void buzzreactive_destroy(const void* key, void* data, void* params) {
   buzzreactive_t reactiv = *(buzzreactive_t*) data;
   if (reactiv->dependents) {
      buzzset_destroy(&reactiv->dependents);
   }
   if (reactiv->fptrlist) {
      buzzdarray_destroy(&reactiv->fptrlist);
   }
   free(reactiv);
}

/****************************************/
/****************************************/

void buzzreactives_destroy(buzzdict_t* reactives) {
   buzzdict_foreach(*reactives, buzzreactive_destroy, NULL);
   buzzdict_destroy(reactives);
}

/****************************************/
/****************************************/
