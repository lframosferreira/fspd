#include <stdio.h>
#include <stdlib.h>

int main(int agrc, char *argv[]){

  if (argc != 1){
    fprintf(stdout, "cli has no arguments\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stdout, "Hello FSPD\n");
  exit(EXIT_SUCCESS);
}
