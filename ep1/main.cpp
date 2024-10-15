#include "passa_tempo.h"
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define MAX_NUMBER_OF_ROOMS 10
#define MAX_NUMBER_OF_THREADS 30

pthread_t threads[MAX_NUMBER_OF_THREADS];
int rooms[MAX_NUMBER_OF_ROOMS + 1];

int S; // number of rooms
int T; // number of threads

typedef struct PositionAttr {
  int room_id;
  int waiting_time;
} PositionAttr;

typedef struct ThreadAttr {
  int id;
  int initial_waiting_time;
  int number_of_rooms_to_visit;
  std::vector<PositionAttr> positions;

  ThreadAttr(int id, int initial_waiting_time, int number_of_rooms_to_visit)
      : id(id), initial_waiting_time(initial_waiting_time),
        number_of_rooms_to_visit(number_of_rooms_to_visit) {
    positions.resize(number_of_rooms_to_visit);
  };
} ThreadAttr;

std::map<int, ThreadAttr> threads_map;

int main(int argc, char *argv[]) {
  if (argc != 1) {
    fprintf(stdout, "cli has no arguments\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stdout, "Hello FSPD\n");

  std::cin >> S >> T;
  for (int i = 0; i < T; i++) {
    int thread_id, initial_waiting_time, number_of_rooms_to_visit;
    std::cin >> thread_id >> initial_waiting_time >> number_of_rooms_to_visit;
    ThreadAttr thread_attr =
        ThreadAttr(thread_id, initial_waiting_time, number_of_rooms_to_visit);
    for (PositionAttr &pos_attr : thread_attr.positions) {
      std::cin >> pos_attr.room_id >> pos_attr.waiting_time;
    }
    threads_map.insert({thread_id, thread_attr});
  }
  exit(EXIT_SUCCESS);
}
