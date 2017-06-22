#include <stdio.h>
#include <stdlib.h>

int main(void) {
  unsigned long int n = 1000000000000;
  int *ptr = malloc(n*sizeof(int));

  for (unsigned int i = 0; i < n; i++){
   ptr[i] = 1;
 }


  return 0;
}
