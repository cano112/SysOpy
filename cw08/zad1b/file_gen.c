#include <stdio.h>

int main(void) {

  FILE *file = fopen("file.bin", "wb");

  for(int i = 0; i < 1000; i++) {
    fwrite(&i, sizeof(i), 1, file);
    for(int j = 0; j<510; j++) fwrite("ab", 2, 1, file);
  }

  fclose(file);




  return 0;
}
