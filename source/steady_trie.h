/*
  The structure is meant to hold 64-bit keys, allowing for insertion, deletion, and searching.
  It is best if the keys are relatively sequential, otherwise it will *not* be memory efficient.

  Honestly, I don't know if this is a trie or a radix tree or *what*... so maybe I should just give it some other kind of name and not worry about whether or not the name am correct.

 TODO:
   [ ] Figure out a better name than just "trie", since "trie" is too generic.
   [ ] Use memory pool for stack-nodes
   [ ] Make persistant
   [x] Allow configuration to store keys least-to-most significant byte or most-to-least.
   [x] Right now the keys are the values, but we probably want the option of having key/value pairs.
   [ ] BUG: Steady_Trie_Slot_Bits == 2  and  Steady_Trie_Root_Is_Lowest_Significant_Byte == 0 causes errors.
*/



////////////////////////////
// Api
////////////////////////////
// TODO: Fill out function/macro defs




////////////////////////////
// Settings
////////////////////////////
#define Steady_Trie_Key_Bits 64
#define Steady_Trie_Slot_Bits 4

#ifndef Steady_Trie_Root_Is_Lowest_Significant_Byte
# define Steady_Trie_Root_Is_Lowest_Significant_Byte 0
#endif

#ifndef Steady_Trie_Use_Key_Value_Pair
# define Steady_Trie_Use_Key_Value_Pair 0
#endif

#if Steady_Trie_Key_Bits == 64
typedef U64 Steady_Trie_Key;
#elif Steady_Trie_Key_Bits == 32
typedef U32 Steady_Trie_Key;
#else
# error Steady_Trie_Key_Bits can only be 32 or 64.
#endif

typedef U8 Steady_Trie_Slot_Type;



////////////////////////////
// Computed Properties
////////////////////////////
#define Steady_Trie_Slot_Count (1 << Steady_Trie_Slot_Bits)
#define Steady_Trie_Max_Depth (Steady_Trie_Key_Bits / Steady_Trie_Slot_Bits)
// TODO: The names for Steady_Trie_Single_Slot_Mask and Steady_Trie_Slot_Mask are confusing. Should probably rename!
#define Steady_Trie_Single_Slot_Mask\
  ((1<<Steady_Trie_Slot_Bits)-1)


////////////////////////////
// Helper Macros
////////////////////////////
#if Steady_Trie_Root_Is_Lowest_Significant_Byte
# define Steady_Trie_Slot_Mask(depth)\
  (~(((U64)1<<((depth+1)*Steady_Trie_Slot_Bits))-1))

# define Steady_Trie_Get_Slot_Shift(depth)\
  (depth)

# define Steady_Trie_Get_Next_Iter_Key_Chunk(key, key_shift, index)\
  (((key) << Steady_Trie_Slot_Bits) | ((index)-1))
#else
# define Steady_Trie_Slot_Mask(depth)\
  (~(max_U64<<(((Steady_Trie_Max_Depth-1)-depth)*Steady_Trie_Slot_Bits)))

# define Steady_Trie_Get_Slot_Shift(depth)\
  ((Steady_Trie_Max_Depth-1)-(depth))

# define Steady_Trie_Get_Next_Iter_Key_Chunk(key, key_shift, index)\
  ((key) | ((U64)((index)-1) << ((key_shift)*Steady_Trie_Slot_Bits)))
#endif // Steady_Trie_Root_Is_Lowest_Significant_Byte

#define Steady_Trie_Get_Initial_Iter_Key_Chunk(index)\
  (index)

#define Steady_Trie_Get_Slot_Value(key, depth)\
  (((key) >> (Steady_Trie_Get_Slot_Shift(depth)*Steady_Trie_Slot_Bits)) &\
   Steady_Trie_Single_Slot_Mask)

#define Steady_Trie_Is_Max_Depth(depth)\
  ((depth)==Steady_Trie_Max_Depth-1)

#define Steady_Trie_Is_Key_At_Final_Depth(key, depth)\
  (Steady_Trie_Is_Max_Depth(depth) || ((Steady_Trie_Slot_Mask(depth) & key) == 0))



////////////////////////////
// Static Invariants
////////////////////////////
StaticAssert((Steady_Trie_Slot_Bits < Steady_Trie_Key_Bits),
             Ensure_Slot_Smaller_Than_Key);
StaticAssert((Steady_Trie_Key_Bits % Steady_Trie_Slot_Bits) == 0,
             Ensure_Slot_Divides_Key);
StaticAssert((8*sizeof(Steady_Trie_Slot_Type) >= Steady_Trie_Slot_Bits),
             Ensure_Slot_Type_Fits_Slot_Bits);
StaticAssert((8*sizeof(Steady_Trie_Key) == Steady_Trie_Key_Bits),
             Ensure_Key_Type_Equals_Key_Bits);

#if Steady_Trie_Use_Key_Value_Pair
# ifndef Steady_Trie_Value_Type
#  error When using Steady_Trie_Use_Key_Value_Pair, Steady_Trie_Value_Type must also be defined.
# endif
#endif






typedef struct Steady_Trie_Node {
  U8 occupied[Steady_Trie_Slot_Count]; // TODO: Bit-flags?
  struct Steady_Trie_Node *slots[Steady_Trie_Slot_Count];
#if Steady_Trie_Use_Key_Value_Pair
  Steady_Trie_Value_Type values[Steady_Trie_Slot_Count];
#endif
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
  Steady_Trie_Key key;
} Steady_Trie_Iterator;

typedef struct Steady_Trie_Root_Stack {
  Steady_Trie_Node *root;
  struct Steady_Trie_Root_Stack *next;
  struct Steady_Trie_Root_Stack *prev;
} Steady_Trie_Root_Stack;

typedef struct Steady_Trie {
  Steady_Trie_Root_Stack *roots;
  Steady_Trie_Node *current_root;
} Steady_Trie;


typedef enum {
  Steady_Trie_Edit_Insert,
  Steady_Trie_Edit_Delete,
  Steady_Trie_Edit_Search,
} Steady_Trie_Edit_Kind;

typedef enum {
  Steady_Trie_Edit_Result_Key,
  Steady_Trie_Edit_Result_Value,
} Steady_Trie_Edit_Result_Kind;

typedef struct Steady_Trie_Edit_Result {
  Steady_Trie_Edit_Result_Kind kind;
  union {
    B32 found;
    Steady_Trie_Value_Type *value;
  };
} Steady_Trie_Edit_Result;



static Steady_Trie_Stack_Node *
steady_trie_create_stack_node(Arena *arena, Steady_Trie_Stack_Node *free_stack) {
  Steady_Trie_Stack_Node *node = 0;
  // TODO: Implement and use

  return node;
}

static void
steady_trie_delete_stack_node(Arena *arena, Steady_Trie_Stack_Node *free_stack, Steady_Trie_Stack_Node *node) {
  // TODO: Implement and use
}

static void steady_trie_iter_next(Steady_Trie_Iterator *iter) {
  // Do a depth-first search until we find the next occupied key.
  for (;;) {
    if (iter->stack && iter->stack->node) {
      if (iter->stack->index < Steady_Trie_Slot_Count) {
        B32 not_visited = ((iter->stack->visited_plus_one == 0) ||
                           (iter->stack->visited_plus_one-1 < iter->stack->index));
        B32 occupied = iter->stack->node->occupied[iter->stack->index];
        if (occupied && not_visited) {
          // NOTE: key_shift is only used for when the root is the most-significant byte.
          U32 key_shift = 1;
          U64 key = Steady_Trie_Get_Initial_Iter_Key_Chunk(iter->stack->index);
          // Build up the key value by collecting all the indices on the stack.
          for (Steady_Trie_Stack_Node *stack_node = iter->stack->next;
               stack_node != 0;
               stack_node = stack_node->next) {
            // NOTE: All of the nodes in the stack, aside from the current one, have had their indices iterated, so we need to subtract one from them. (This is alway why we do not process the current stack-node in the loop.)
            key = Steady_Trie_Get_Next_Iter_Key_Chunk(key, key_shift, stack_node->index);
            key_shift += 1;
          }
          iter->key = key;
          iter->stack->visited_plus_one = iter->stack->index+1;
          break;
        }
        else if (iter->stack->node->slots[iter->stack->index]) {
          // Child slot is present, so descend.
          // TODO: Recycle free stack-nodes!!!!!
          Steady_Trie_Node *next_node = iter->stack->node->slots[iter->stack->index];
          Steady_Trie_Stack_Node *new_stack_node = arena_push(iter->arena, sizeof(Steady_Trie_Stack_Node));
          iter->stack->index += 1;
          new_stack_node->node = next_node;
          SLLStackPush(iter->stack, new_stack_node);
        }
        else {
          iter->stack->index += 1;
        }
      }
      else {
        // Reached the end of the current node's slots, so go back up.
        SLLStackPop(iter->stack); // TODO: Recycle stack-nodes!!!!!!!!
      }
    }
    else {
      // The iter-stack or the stack's node is empty, so bail.
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
    printf("  key %llu\n", (U64)iter_name->key);
  }
}


static void steady_trie_print_trie_as_dot_file(Arena *arena, Steady_Trie_Node *root) {
  // TODO: Use it or lose it.
  printf("\n\ndigraph G {\n");
  printf("rankdir=\"LR\";\n");
  Steady_Trie_Iterator *iter = steady_trie_iter_init(arena, root);

  for (;;) {
    if (iter->stack && iter->stack->node) {
      if (iter->stack->index < Steady_Trie_Slot_Count) {
        if (iter->stack->node->slots[iter->stack->index]) {
          // TODO: Recycle free stack-nodes!!!!!
          Steady_Trie_Node *next_node = iter->stack->node->slots[iter->stack->index];
          Steady_Trie_Stack_Node *new_stack_node = arena_push(iter->arena, sizeof(Steady_Trie_Stack_Node));
          iter->stack->index += 1;
          new_stack_node->node = next_node;
          printf("%llu->%llu;\n", (U64)iter->stack->node, (U64)new_stack_node->node);
          SLLStackPush(iter->stack, new_stack_node);
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

  printf("}\n\n\n");
}




static Steady_Trie_Edit_Result
steady_trie_edit(Arena *arena,
                 Steady_Trie_Node *root,
                 Steady_Trie_Key key,
                 Steady_Trie_Value_Type value,
                 Steady_Trie_Edit_Kind edit_kind) {
  Steady_Trie_Edit_Result result = (Steady_Trie_Edit_Result){0};
  Steady_Trie_Node *node = root;

  for (U32 d = 0; (d < Steady_Trie_Max_Depth) && node; ++d) {
    Steady_Trie_Slot_Type slot_value = Steady_Trie_Get_Slot_Value(key, d);
    Assert(slot_value >= 0 && slot_value <= Steady_Trie_Single_Slot_Mask);

    if (Steady_Trie_Is_Key_At_Final_Depth(key, d)) {
      if (edit_kind == Steady_Trie_Edit_Insert) {
        node->occupied[slot_value] = 1;
#if Steady_Trie_Use_Key_Value_Pair
        node->values[slot_value] = value;
#endif
      }
      else if (edit_kind == Steady_Trie_Edit_Delete) {
        node->occupied[slot_value] = 0;
      }
      else if (edit_kind == Steady_Trie_Edit_Search) {
#if Steady_Trie_Use_Key_Value_Pair
        if (node->occupied[slot_value]) {
          result.value = &node->values[slot_value];
        }
#else
        result.found = node->occupied[slot_value] ? 1 : 0;
#endif
      }
      break;
    }
    else {
      if (edit_kind == Steady_Trie_Edit_Insert && node->slots[slot_value] == 0) {
        // Add new node
        node->slots[slot_value] = arena_push(arena, sizeof(Steady_Trie_Node));
      }

      // Descend
      node = node->slots[slot_value];
    }
  }

  return result;
}



#if Steady_Trie_Use_Key_Value_Pair
static Steady_Trie_Edit_Result steady_trie_set(Arena *arena, Steady_Trie_Node *root, Steady_Trie_Key key, Steady_Trie_Value_Type value) {
  return steady_trie_edit(arena, root, key, value, Steady_Trie_Edit_Insert);
}
#else
static Steady_Trie_Edit_Result steady_trie_insert(Arena *arena, Steady_Trie_Node *root, Steady_Trie_Key key) {
  return steady_trie_edit(arena, root, key, 0, Steady_Trie_Edit_Insert);
}
#endif

static Steady_Trie_Edit_Result steady_trie_delete(Steady_Trie_Node *root, Steady_Trie_Key key) {
  return steady_trie_edit(0, root, key, 0, Steady_Trie_Edit_Delete);
}

static Steady_Trie_Edit_Result steady_trie_search(Steady_Trie_Node *root, Steady_Trie_Key key) {
  return steady_trie_edit(0, root, key, 0, Steady_Trie_Edit_Search);
}
