#include <stdio.h>

int test_component_registry(void);
int test_component_compose(void);

int main(void) {
  if (test_component_registry() != 0) {
    return 1;
  }
  if (test_component_compose() != 0) {
    return 1;
  }
  printf("compiler tests passed\n");
  return 0;
}
