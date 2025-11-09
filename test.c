#define MR4TH_ASSERTS 1

#define Test_Rb_Tree 1
#define Test_Pers_Rb_Tree 0

#if Test_Rb_Tree
# include "rbtree.c"
#endif

#if Test_Pers_Rb_Tree
# include "persistent_rbtree.c"
#endif

#define ArrayCount(a) (sizeof(a)/sizeof(*(a)))

#if !defined(AssertBreak)
# define AssertBreak() (*(volatile int*)0 = 0)
#endif

#define Stmnt(S) do{ S }while(0)
#if MR4TH_ASSERTS
# define Assert(c) Stmnt( if (!(c)){ AssertBreak(); } )
#else
# define Assert(c)
#endif

int main(void) {
  RBTree tree = {0};
  size_t values[] = {
    234, 54, 1, 87, 5, 4, 90, 54362, 3, 645, 3245, 34
  };
  size_t remove_indices[] = { 2, 6, 10, 4, 7 };

  /* print_rb_tree(tree.root); */

  for(size_t i = 0; i < ArrayCount(values); i++) {
    size_t value = values[i];
    size_t key = value;
    printf("inserting k=%zu v=%zu\n", key, value);
    rb_insert(&tree, key, value);
  }
  /* print_rb_tree(tree.root); */

  for(size_t i = 0; i < ArrayCount(remove_indices); i++) {
    size_t index = remove_indices[i];
    Assert(index >= 0 && index < ArrayCount(values));
    size_t value = values[index];
    printf("deleting v=%zu\n", value);
    rb_delete(&tree, value);
  }
  /* print_rb_tree(tree.root); */

  printf("\n\nFinding original inserted values\n");
  for(size_t i = 0; i < ArrayCount(values); i++) {
    size_t key = values[i];
    Value result = 0;
    int found = rb_find(&tree, key, &result);
    printf("  %zu %s\n", key, found?"found":"not found");
  }

}
