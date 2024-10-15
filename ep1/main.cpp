#include <stdio.h>
#include <stdlib.h>
#include "passa_tempo.h"

int S; // number of rooms
int T; // number of threads

typedef struct PositionAttr{
    int room_id;
    int waiting_time;
} PositionAttr;

typedef struct ThreadAttr {
    int id;
    int initial_waiting_time;
    int number_of_rooms_to_visit;
    std::vector<PositionAttr> positions;
}
ThreadAttr;

int main(int argc, char *argv[]){

  if (argc != 1){
    fprintf(stdout, "cli has no arguments\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stdout, "Hello FSPD\n");
  exit(EXIT_SUCCESS);
}
