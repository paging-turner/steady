/*
  This list/queue implementation copies nodes up to the root when necessary.
  Stack pushes should be efficient while queue pushes are *not* efficient.
*/

#ifndef Steady_Queue_Copy_Value_Type
# define Steady_Queue_Copy_Value_Type void*
#endif



typedef struct Steady_Queue_Copy_Node Steady_Queue_Copy_Node;

struct Steady_Queue_Copy_Node {
  Steady_Queue_Copy_Value_Type value;
  Steady_Queue_Copy_Node *next;
  Steady_Queue_Copy_Node *prev;
};

typedef struct Steady_Queue_Copy_Version Steady_Queue_Copy_Version;
struct Steady_Queue_Copy_Version {
  Steady_Queue_Copy_Node *first;
  Steady_Queue_Copy_Node *last;
  Steady_Queue_Copy_Version *next_version;
};

typedef struct {
  Steady_Queue_Copy_Version *first_version;
  Steady_Queue_Copy_Version *last_version;
} Steady_Queue_Copy;



void steady_queue_copy_push(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Copy_Node *node) {
  Assert(0);
}
