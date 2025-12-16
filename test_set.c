#include <stdio.h>

#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "source/steady_set.h"


S32 main(void) {
  Arena *arena = arena_alloc_reserve(1024*1024, 0);

  Steady_Set_Node *root = arena_push(arena, sizeof(Steady_Set_Node));
  root->kind = Steady_Set_Node_Kind_Root;
  Steady_Set_Node *node_0 = arena_push(arena, sizeof(Steady_Set_Node));
  node_0->kind = Steady_Set_Node_Kind_Inner;
  Steady_Set_Node *node_1 = arena_push(arena, sizeof(Steady_Set_Node));
  node_1->kind = Steady_Set_Node_Kind_Inner;
  Steady_Set_Node *node_2 = arena_push(arena, sizeof(Steady_Set_Node));
  node_2->kind = Steady_Set_Node_Kind_Leaf;
  node_2->items[0].value = 42;
  node_2->items[1].value = 987;
  Steady_Set_Node *node_3 = arena_push(arena, sizeof(Steady_Set_Node));
  node_3->kind = Steady_Set_Node_Kind_Leaf;
  node_3->items[0].value = 1234;
  node_3->items[1].value = 478;
  node_3->items[2].value = 874;
  Steady_Set_Node *node_4 = arena_push(arena, sizeof(Steady_Set_Node));
  node_4->kind = Steady_Set_Node_Kind_Leaf;
  node_4->items[0].value = 783;
  node_4->items[1].value = 20;
  node_4->items[2].value = 59;

  root->children[0] = node_0;
  root->children[1] = node_1;
  node_0->children[0] = node_2;
  node_1->children[0] = node_3;
  node_1->children[1] = node_4;

  steady_set_validate(arena, root);

  return 0;
}
