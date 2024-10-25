#include "passa_tempo.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#define MAX_NUMBER_OF_ROOMS 10
#define MAX_NUMBER_OF_THREADS 30

#define dbg(x) std::cout << #x << " = " << x << std::endl

typedef struct {
  int are_inside;
  int wants_in;
} RoomThreads;

int rooms[MAX_NUMBER_OF_ROOMS + 1];
RoomThreads rooms_threads[MAX_NUMBER_OF_ROOMS + 1];
pthread_mutex_t mutex;

int S; // number of rooms
int T; // number of threads

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

pthread_cond_t room_full_cond[MAX_NUMBER_OF_ROOMS + 1];
pthread_cond_t room_three_in_queue_cond[MAX_NUMBER_OF_ROOMS + 1];
pthread_cond_t room_three_ready_to_go_cond[MAX_NUMBER_OF_ROOMS + 1];

pthread_mutex_t rooms_threads_mutex[MAX_NUMBER_OF_ROOMS + 1];

void init() {
  for (int i = 1; i <= S; i++) {
    pthread_cond_init(&room_full_cond[i], NULL);
    pthread_cond_init(&room_three_in_queue_cond[i], NULL);
    pthread_cond_init(&room_three_ready_to_go_cond[i], NULL);
  }
  memset(rooms, 0, sizeof(rooms));
  memset(rooms_threads, 0, sizeof(rooms_threads));
  for (int i = 1; i <= S; i++) {
    pthread_mutex_init(&rooms_threads_mutex[i], NULL);
  }
}

void entra(int room_id) {
  pthread_mutex_lock(&rooms_threads_mutex[room_id]);
  auto &[are_inside, wants_in] = rooms_threads[room_id];

  while (1) {
    while (are_inside > 0) {
      pthread_cond_wait(&room_full_cond[room_id],
                        &rooms_threads_mutex[room_id]);
    }
    wants_in++;
    if (wants_in < 3) {
      pthread_cond_wait(&room_three_in_queue_cond[room_id],
                        &rooms_threads_mutex[room_id]);
      break;
    } else {
      are_inside = 3;
      if (wants_in > 3)
        continue;
    }
    if (wants_in == 3) {
      pthread_cond_broadcast(&room_three_in_queue_cond[room_id]);
    }
    wants_in = 0;
    break;
  }

  pthread_mutex_unlock(&rooms_threads_mutex[room_id]);
}

void sai(int room_id) {

  // se ja acabei as salas nem preocupo
  pthread_mutex_lock(&rooms_threads_mutex[room_id]);
  auto &[are_inside, wants_in] = rooms_threads[room_id];
  are_inside--;
  if (are_inside == 0) {
    pthread_cond_broadcast(&room_full_cond[room_id]);
  }
  pthread_mutex_unlock(&rooms_threads_mutex[room_id]);
};

void *func(void *thread_id_ptr) {
  int thread_id = *((int *)thread_id_ptr);
  free(thread_id_ptr);

  auto [initial_waiting_time, number_of_rooms_to_visit, positions] =
      threads_attr_map.at(thread_id);
  passa_tempo(thread_id, 0, initial_waiting_time);

  for (int i = 0; i < positions.size(); i++) {
    auto [room_id, waiting_time] = positions.at(i);
    entra(room_id); // só entro se eu nao estiver saindo da ultima posição
    if (i != 0) {
      sai(positions.at(i - 1).room_id);
    }
    passa_tempo(thread_id, room_id, waiting_time);
  }
  sai(positions.back().room_id);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  std::ios_base::sync_with_stdio(0);
  std::cin.tie(0);
  if (argc != 1) {
    std::cerr << "cli has no arguments, you should read only from stdin\n";
    exit(EXIT_FAILURE);
  }
  init();
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
    if (pthread_create(&threads.at(thread_id), NULL, func, thread_id_ptr) !=
        0) {
      free(thread_id_ptr);
      std::cerr << "error creating thread\n";
      exit(EXIT_FAILURE);
    }
  }
  for (auto &[id, t] : threads) {
    pthread_join(t, NULL);
  }
  exit(EXIT_SUCCESS);
}
