#include <stdio.h>

#define MR4TH_ASSERTS 1
#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#define Steady_Arena  Arena
#define steady_arena_push_size(arena, size)  arena_push(arena, size)

#define Steady_Queue_Slow_Value_Type int
#include "steady_list_slow.h"

#define Steady_Queue_Copy_Value_Type int
#include "steady_list_copy.h"

#define Steady_Queue_Value_Type int
#include "steady_list.h"



void debug_print_steady_queue(Steady_Queue q) {
  for (Steady_Queue_Node *n = q.first; n != 0; n = n->next) {
    printf("[%d]", n->value);
    if (n->next) {
      printf("->");
    }
  }
  printf("\n");
}

int main(void) {
  Arena *slow_arena = arena_alloc_reserve(1024*1024, 0);

#if 0
  Steady_Queue queue = (Steady_Queue){0};
  Steady_Queue_Node node_a = (Steady_Queue_Node){0};
  node_a.value = 32;
  Steady_Queue_Node node_b = (Steady_Queue_Node){0};
  node_b.value = 123;
  Steady_Queue_Node node_c = (Steady_Queue_Node){0};
  node_c.value = 999;

  debug_print_steady_queue(queue);
  steady_queue_push(&queue, &node_a);
  debug_print_steady_queue(queue);
  steady_queue_push(&queue, &node_b);
  debug_print_steady_queue(queue);
  steady_queue_push_front(&queue, &node_c);
  debug_print_steady_queue(queue);
#elif 1
  Steady_Queue_Slow queue = (Steady_Queue_Slow){0};
  Steady_Queue_Slow_Node node_a = (Steady_Queue_Slow_Node){0};
  node_a.value = 32;
  Steady_Queue_Slow_Node node_b = (Steady_Queue_Slow_Node){0};
  node_b.value = 123;
  Steady_Queue_Slow_Node node_c = (Steady_Queue_Slow_Node){0};
  node_c.value = 999;

  steady_queue_slow_push(slow_arena, &queue, &node_a);
  steady_queue_slow_push(slow_arena, &queue, &node_b);
  steady_queue_slow_push_front(slow_arena, &queue, &node_c);
  steady_queue_slow_delete(slow_arena, &queue, node_a.id);
  steady_queue_slow_pop(slow_arena, &queue);

  S32 version_index = 0;
  for (Steady_Queue_Slow_Version *v = queue.first_version; v != 0; v = v->next_version) {
    printf("Version %2d: ", version_index++);
    for (Steady_Queue_Slow_Node *n = v->first; n != 0; n = n->next) {
      printf("%d ", n->value);
    }
    printf("\n");
  }
#else
  Steady_Queue_Copy queue = (Steady_Queue_Copy){0};
  Steady_Queue_Copy_Node node_a = (Steady_Queue_Copy_Node){0};
  node_a.value = 32;
  Steady_Queue_Copy_Node node_b = (Steady_Queue_Copy_Node){0};
  node_b.value = 123;
  Steady_Queue_Copy_Node node_c = (Steady_Queue_Copy_Node){0};
  node_c.value = 999;

  steady_queue_copy_push(slow_arena, &queue, &node_a);
  steady_queue_copy_push(slow_arena, &queue, &node_b);
  steady_queue_copy_push(slow_arena, &queue, &node_c);

  S32 version_index = 0;
  for (Steady_Queue_Copy_Version *v = queue.first_version; v != 0; v = v->next_version) {
    printf("Version %2d: ", version_index++);
    for (Steady_Queue_Copy_Node *n = v->first; n != 0; n = n->next) {
      printf("%d ", n->value);
    }
    printf("\n");
  }
#endif

  return 0;
}
