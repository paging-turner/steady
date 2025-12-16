#define Steady_Trie_Key_Bits 64
#define Steady_Trie_Slot_Bits 8
typedef U64 Steady_Trie_Key;
typedef U8 Steady_Trie_Slot_Type;

/* #ifndef Steady_Trie_Value_Type */
/* # error Please define a type for Steady_Trie_Value_Type */
/* #endif */

StaticAssert((Steady_Trie_Slot_Bits < Steady_Trie_Key_Bits),
             Ensure_Slot_Smaller_Than_Key);
StaticAssert((Steady_Trie_Key_Bits % Steady_Trie_Slot_Bits) == 0,
             Ensure_Slot_Divides_Key);
StaticAssert((8*sizeof(Steady_Trie_Slot_Type) >= Steady_Trie_Slot_Bits),
             Ensure_Slot_Type_Fits_Slot_Bits);
StaticAssert((8*sizeof(Steady_Trie_Key) == Steady_Trie_Key_Bits),
             Ensure_Key_Type_Equals_Key_Bits);

#define Steady_Trie_Slot_Count (1 << Steady_Trie_Slot_Bits)
#define Steady_Trie_Max_Depth (Steady_Trie_Key_Bits / Steady_Trie_Slot_Bits)

typedef struct Steady_Trie_Node {
  U8 occupied[Steady_Trie_Slot_Count]; // TODO: Bit-flags?
  struct Steady_Trie_Node *slots[Steady_Trie_Slot_Count];
} Steady_Trie_Node;

typedef struct Steady_Trie_Stack_Node {
  struct Steady_Trie_Stack_Node *next;
  Steady_Trie_Node *node;
  U32 index;
  U32 visited_plus_one;
} Steady_Trie_Stack_Node;

typedef struct Steady_Trie_Iterator {
  Arena *arena;
  Steady_Trie_Stack_Node *stack;
  Steady_Trie_Stack_Node *free_stack; // TODO: Use free stack-nodes!
  /* U32 depth; */
  Steady_Trie_Key key;
} Steady_Trie_Iterator;


static void steady_trie_iter_next(Steady_Trie_Iterator *iter) {
  for (;;) {
    if (iter->stack && iter->stack->node) {
      if (iter->stack->index < Steady_Trie_Slot_Count) {
        B32 not_visited = ((iter->stack->visited_plus_one == 0) ||
                           (iter->stack->visited_plus_one-1 < iter->stack->index));
        if (iter->stack->node->occupied[iter->stack->index] && not_visited) {
          U64 key = 0;
          U32 depth = 0;
          for (Steady_Trie_Stack_Node *stack_node = iter->stack;
               stack_node != 0;
               stack_node = stack_node->next) {
            key = (key << (depth*Steady_Trie_Slot_Bits)) | iter->stack->index;
          }
          iter->key = key;
          iter->stack->visited_plus_one = iter->stack->index+1;
          /* iter->stack->index += 1; */
          break;
        }
        else if (iter->stack->node->slots[iter->stack->index]) {
          // TODO: Recycle free stack-nodes!!!!!
          Steady_Trie_Node *next_node = iter->stack->node->slots[iter->stack->index];
          Steady_Trie_Stack_Node *new_stack_node = arena_push(iter->arena, sizeof(Steady_Trie_Stack_Node));
          iter->stack->index += 1;
          SLLStackPush(iter->stack, new_stack_node);
          new_stack_node->node = next_node;
        }
        else {
          iter->stack->index += 1;
        }
      }
      else {
        SLLStackPop(iter->stack); // TODO: Recycle stack-nodes!!!!!!!!
      }
    }
    else {
      break;
    }
  }
}

static Steady_Trie_Iterator *steady_trie_iter_init(Arena *arena, Steady_Trie_Node *root) {
  Steady_Trie_Iterator *iter = arena_push(arena, sizeof(Steady_Trie_Iterator));
  Steady_Trie_Stack_Node *stack = arena_push(arena, sizeof(Steady_Trie_Stack_Node));

  if (iter && stack) {
    iter->arena = arena;
    iter->stack = stack;
    iter->stack->node = root;
  }

  steady_trie_iter_next(iter);

  return iter;
}

static B32 steady_trie_iter_test(Steady_Trie_Iterator *iter) {
  return iter && iter->stack;
}

#define Steady_Trie_Iterate(iter_name, arena, root)\
  for (Steady_Trie_Iterator *iter_name = steady_trie_iter_init(arena, root);\
       steady_trie_iter_test(iter_name);\
       steady_trie_iter_next(iter_name))

static void steady_trie_print_trie(Arena *arena, Steady_Trie_Node *root) {
  printf("trie %p\n", root);
  Steady_Trie_Iterate(iter_name, arena, root) {
    printf("  key %llx\n", iter_name->key);
  }
}

static void steady_trie_print_trie_as_dot_file(Steady_Trie_Node *root) {
  printf("digraph G {\n");
  {
    printf("%p;\n", root);
  }
  printf("}\n");
}
