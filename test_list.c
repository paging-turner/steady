#include <stdio.h>

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

  return 0;
}
