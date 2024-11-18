// Exercício de programação 1 - FSPD
// Aluno: Luís Felipe Ramos Ferreira
// Matrícula: 2019022553

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

// Estrutura utilizada para armazenar contexto atual de uma sala em relação
// às threads que estão dentro dela e as que querem entrar
typedef struct {
  // Número de threads dentro da sala
  int are_inside;
  // Número de threads que querem entrar na sala
  int wants_in;
} RoomThreads;

// Vetores globais utilizados no projeto

// Salas
int rooms[MAX_NUMBER_OF_ROOMS + 1];

// Contexto de cada sala
RoomThreads rooms_threads[MAX_NUMBER_OF_ROOMS + 1];

int S; // Número de salas
int T; // Número de threads

// Estrutura que armazena os atributos de cada posição/sala
// Essa estrutura é utilizada por posição que cada thread deve passar
typedef struct PositionAttr {
  // Identificador da posição/sala
  int room_id;
  // Quanto tempo a thread deve esperar quando dentro dessa sala
  int waiting_time;
} PositionAttr;

// Estrutura que armazena os atributos de cada thread do sistema
typedef struct ThreadAttr {
  // Tempo inicial que a thread deve esperar antes de tentar entrar em uma sala
  int initial_waiting_time;
  // Número de salas que a thread deve visitar
  int number_of_rooms_to_visit;
  // Vetor de posições na ordem em que a thread deve visitá-las. Armazenam
  // estruturas do tipo PosisitionAttr
  std::vector<PositionAttr> positions;

  // Construtor da estrutura
  // Fora a inicialização de variáveis simples, inicializa o vetor de posições
  // com o tamanho igual ao número de salas que a thread deverá visitar
  ThreadAttr(int initial_waiting_time, int number_of_rooms_to_visit)
      : initial_waiting_time(initial_waiting_time),
        number_of_rooms_to_visit(number_of_rooms_to_visit) {
    positions.resize(number_of_rooms_to_visit);
  };

} ThreadAttr;

// Operator Overloading para facilitar que as informações da estrutura
// ThreadAttr sejam impressas na saída padrão
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

// Dicionário que armazena os atributos de cada thread, indexados pelo
// identificador de cada thread
std::map<int, ThreadAttr> threads_attr_map;

// Dicionário que armazena cada thread, indexadas pelo identificador de cada
// thread
std::map<int, pthread_t> threads;

// Vetores de variáveis condicionais para cada sala

// Essa condição armazena a informação de que uma sala já está cheia ou não
pthread_cond_t room_full_cond[MAX_NUMBER_OF_ROOMS + 1];

// Essa condição armazena a informação de que já existem três threads na espera
// para entrar na sala
pthread_cond_t room_three_in_queue_cond[MAX_NUMBER_OF_ROOMS + 1];

// Vetor de mutexes para cada sala
pthread_mutex_t rooms_threads_mutex[MAX_NUMBER_OF_ROOMS + 1];

// Função de inicialização de todas as variáveis necessárias para funcionamento
// do código. Não recebe nada como entrada nem retorna nada, apenas inicializa
// mutexes, variáveis de condição, etc
void init() {
  for (int i = 1; i <= S; i++) {
    pthread_cond_init(&room_full_cond[i], NULL);
    pthread_cond_init(&room_three_in_queue_cond[i], NULL);
  }
  memset(rooms, 0, sizeof(rooms));
  memset(rooms_threads, 0, sizeof(rooms_threads));
  for (int i = 1; i <= S; i++) {
    pthread_mutex_init(&rooms_threads_mutex[i], NULL);
  }
}

// Função para que a thread entre na sala
// Entrada: identificador da sala que a thread deve entrar
// Saída: nenhuma
void entra(int room_id) {
  pthread_mutex_lock(&rooms_threads_mutex[room_id]);
  auto &[are_inside, wants_in] = rooms_threads[room_id];

  // Loop principal da tentativa de entrada em uma sala por parte de uma thread
  while (1) {
    // Enquanto a sala estiver ocupada, nenhuma thread pode entrar na sala,
    // portanto todas devem esperar até que a sala fique vazia. Para isso,
    // utilizamos mais um laço juntamente com uma variável condicional.
    while (are_inside > 0) {
      pthread_cond_wait(&room_full_cond[room_id],
                        &rooms_threads_mutex[room_id]);
    }
    // Quando o código de uma thread chega aqui, sabemos que mais uma thread
    // quer entrar, por isso incrementamos a variável `wants_in` em uma unidade.
    wants_in++;
    // Se `wants_in` for menor que três, a thread deve esperar, uma vez que
    // threads não podem entrar em uma sala a menos que seja em um trio. Para
    // essa espera, utilizamos um if atrelado à uma variável condicional
    if (wants_in < 3) {
      pthread_cond_wait(&room_three_in_queue_cond[room_id],
                        &rooms_threads_mutex[room_id]);
      // Aqui, utilizamos um break para que as threads que podem entrar na sala
      // não fiquem presas no loop while externo
      break;
    } else {
      // Caso `wants_in` seja maior ou igual a 3, temos dois casos
      // Se for igual a 3, o primeiro if é desconsiderado, e a variável
      // `are_inside` fica com valor 3. Isso porque agora que existem três
      // threads prontas para entrar em uma sala vazia, podemos colocar as
      // threads lá e a sala passa a conter as 3. Quando `wants_in` é maior que
      // 3, apenas voltamos para o laço incial. Com isso, as thread excessivas
      // que queriam entrar mas chegaram atrasadas, isto é, não chegaram para
      // ser parte do trio inicial, voltam à esperar no loop de espera para que
      // a sala esteja vazia.
      if (wants_in > 3)
        continue;
      are_inside = 3;
    }
    // Se `wants_in` for 3, podemos utilizar um broadcast para avisar as threads
    // esperando na variável de condição da linha 138 que elas podem continuar a
    // execução
    if (wants_in == 3) {
      pthread_cond_broadcast(&room_three_in_queue_cond[room_id]);
    }
    // Agora, podemos marcar `wants_in` com 0, porque as threads que queriam
    // entrar estão ou esperando que a sala fique vazia ou estão dentro da sala.
    wants_in = 0;
    break;
  }
  pthread_mutex_unlock(&rooms_threads_mutex[room_id]);
}

// Função para uma thread sair de uma sala
// Entrada: identificador da sala
// Saída: nenhuma
void sai(int room_id) {
  pthread_mutex_lock(&rooms_threads_mutex[room_id]);
  auto &[are_inside, wants_in] = rooms_threads[room_id];
  // Qaundo saímos de uma sala, diminuímos o contador do número de threads
  // dentro dessa sala.
  are_inside--;
  // Se `are_inside` para aquela sala for 0, utilizamos um broadcast para avisar
  // as threads que querem entrar naquela sala que ela está vazia
  if (are_inside == 0) {
    // wants_in = 0;
    pthread_cond_broadcast(&room_full_cond[room_id]);
  }
  pthread_mutex_unlock(&rooms_threads_mutex[room_id]);
};

// Função que cada thread irá executar
// Entrada: ponteiro para identificador da thread
// Saída: ponteiro para o tipo void
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

  return 0;
}

int main(int argc, char *argv[]) {
  // Se número de parâmetros de linha de comando for diferente de 1,
  // consideramos um erro, pois toda a entrada deve ser lida da entrada padrão.
  if (argc != 1) {
    std::cerr << "cli has no arguments, you should read only from stdin\n";
    exit(EXIT_FAILURE);
  }
  // Inicialização das variáveis com a função init()
  init();
  // Leitura do número de salas e threads da entrada padrão
  std::cin >> S >> T;
  // Loop para leitura dos atributos de cada thread e também a criação das
  // estruturas e das threads
  for (int i = 0; i < T; i++) {
    int thread_id, initial_waiting_time, number_of_rooms_to_visit;
    std::cin >> thread_id >> initial_waiting_time >> number_of_rooms_to_visit;
    ThreadAttr thread_attr =
        ThreadAttr(initial_waiting_time, number_of_rooms_to_visit);
    for (PositionAttr &pos_attr : thread_attr.positions) {
      std::cin >> pos_attr.room_id >> pos_attr.waiting_time;
    }
    threads_attr_map.insert({thread_id, thread_attr});

    // Criação de cada thread
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
  // Join das threads
  for (auto &[id, t] : threads) {
    pthread_join(t, NULL);
  }
  exit(EXIT_SUCCESS);
}
