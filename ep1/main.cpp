#include "passa_tempo.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#define MAX_NUMBER_OF_ROOMS 10
#define MAX_NUMBER_OF_THREADS 30

int rooms[MAX_NUMBER_OF_ROOMS + 1];
int rooms_thread_count[MAX_NUMBER_OF_ROOMS + 1];

pthread_mutex_t mutex;
pthread_cond_t empty_room;

int S; // number of rooms
int T; // number of threads

void dbg(const char *txt) { fprintf(stdout, "%s\n", txt); }

typedef struct PositionAttr {
  int room_id;
  int waiting_time;
} PositionAttr;

typedef struct ThreadAttr {
  int initial_waiting_time;
  int number_of_rooms_to_visit;
  std::vector<PositionAttr> positions;

  ThreadAttr(int initial_waiting_time, int number_of_rooms_to_visit)
      : initial_waiting_time(initial_waiting_time),
        number_of_rooms_to_visit(number_of_rooms_to_visit) {
    positions.resize(number_of_rooms_to_visit);
  };

} ThreadAttr;

std::ostream &operator<<(std::ostream &os, const ThreadAttr &thread_attr) {
  os << "initial waiting time: " << thread_attr.initial_waiting_time
     << std::endl;
  os << "number_of_rooms_to_visit: " << thread_attr.number_of_rooms_to_visit
     << std::endl;
  os << "positions: " << std::endl;
  for (const auto &[pos_id, waiting_time] : thread_attr.positions) {
    os << pos_id << " " << waiting_time << std::endl;
  }
  return os;
}

std::map<int, ThreadAttr> threads_attr_map;
std::map<int, pthread_t> threads;

void *func(void *thread_id_ptr) {
  int thread_id = *((int *)thread_id_ptr);
  free(thread_id_ptr);

  auto [initial_waiting_time, number_of_rooms_to_visit, positions] =
      threads_attr_map.at(thread_id);
  passa_tempo(thread_id, 0, initial_waiting_time);

  int idx = -1;
  while (idx + 1 < positions.size()) {

    idx++;
    auto [room_id, waiting_time] = positions.at(idx);
    passa_tempo(thread_id, room_id, waiting_time);
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  if (argc != 1) {
    fprintf(stdout, "cli has no arguments, you should read only from stdin\n");
    exit(EXIT_FAILURE);
  }
  memset(rooms, 0, sizeof(rooms));
  memset(rooms_thread_count, 0, sizeof(rooms_thread_count));
  std::cin >> S >> T;
  for (int i = 0; i < T; i++) {
    int thread_id, initial_waiting_time, number_of_rooms_to_visit;
    std::cin >> thread_id >> initial_waiting_time >> number_of_rooms_to_visit;
    ThreadAttr thread_attr =
        ThreadAttr(initial_waiting_time, number_of_rooms_to_visit);
    for (PositionAttr &pos_attr : thread_attr.positions) {
      std::cin >> pos_attr.room_id >> pos_attr.waiting_time;
    }
    threads_attr_map.insert({thread_id, thread_attr});
    threads.insert({thread_id, pthread_t()});
    int *thread_id_ptr = (int *)malloc(sizeof(int));
    *thread_id_ptr = thread_id;
    pthread_t t;
    if (pthread_create(&threads.at(thread_id), NULL, func, thread_id_ptr) !=
        0) {
      fprintf(stderr, "Error creating thread\n");
      exit(EXIT_FAILURE);
    }
    for (auto &[id, t] : threads) {
      pthread_join(t, NULL);
    }
  }
  exit(EXIT_SUCCESS);
}
