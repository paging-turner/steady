#include <stdint.h>

#ifndef Steady_Queue_Value_Type
# define Steady_Queue_Value_Type void*
#endif

typedef struct Steady_Queue_Node Steady_Queue_Node;

struct Steady_Queue_Node {
  Steady_Queue_Value_Type value;
  Steady_Queue_Node *next;
  Steady_Queue_Node *next_version;
};

typedef struct {
  Steady_Queue_Node *first;
  Steady_Queue_Node *last;
} Steady_Queue;

void steady_queue_push(Steady_Queue *queue, Steady_Queue_Node *node) {
  if (queue->first == 0 || queue->last == 0) {
    queue->first = queue->last = node;
  } else {
    queue->last->next = node;
    queue->last = node;
  }
  node->next = 0;
}

void steady_queue_push_front(Steady_Queue *queue, Steady_Queue_Node *node) {
  if (queue->first == 0 || queue->last == 0) {
    queue->first = queue->last = node;
    node->next = 0;
  } else {
    node->next = queue->first;
    queue->first = node;
  }
}
