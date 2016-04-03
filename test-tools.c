#include "tools.h"
#include<stdio.h>

void test_get_bits();

int main() {
  test_get_bits();
  return 0;
}
void test_get_bits() {
  if(get_bits(0xffffff00, 0, 7) == 0) {
    printf("Test 1 pass");
  } else {
    printf("Test 1 fail value expected 0, got: %x", get_bits(0xffffff00, 0, 7));
  }

}
