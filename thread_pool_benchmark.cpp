#include <iostream>
#include <sstream>

#include "callback.hpp"
#include "thread_pool_normal.hpp"
#include "thread_pool_fast.hpp"
#include "timer.hpp"

namespace {

using base::Callback;
using base::makeCallableMany;
using base::ThreadPoolNormal;
using base::ThreadPoolFast;
using base::Timer;


class Server {
public:
  Server(int initial_value) {
    value = initial_value;
  }
  ~Server() {
  }

  int getValue() {
    return value;
  }


  void SlowAccumulate(int limit) {
    for (int i=0; i<limit; i++) {
      value = i;
    }
  }

  void FastAccumulate(int limit) {
    for (int i=0; i<limit; i++) {
      value =  i;
    }
  }
private:
  int value;
};

Timer timer;

template<typename PoolType>
void FastConsumer() {  
  const int NumServers = 10;
  Server* servers[NumServers];
  for (int i=0; i<NumServers; i++) {
    servers[i] = new Server(i);
  }
  Callback<void>* tasks[NumServers];

  for (int i=0; i<NumServers; i++) {
    tasks[i] = makeCallableMany(&Server::FastAccumulate, servers[i], 10000);
  }
  PoolType* pool = new PoolType(10);
  timer.reset();
  timer.start();
  for (int i=0; i<1000; i++) {
    pool->addTask(tasks[i%NumServers]);
  }
  pool->stop();
  timer.end();
  std::cout << "Fast Consumer:\t";
  std::cout << timer.elapsed();
  std::cout << std::endl;
  for (int i=0; i<NumServers; i++) {
    delete tasks[i];
    delete servers[i];
  }
  delete pool;
}


template<typename PoolType>
void SlowConsumer() {  
  const int NumServers = 10;
  Server* servers[NumServers];
  for (int i=0; i<NumServers; i++) {
    servers[i] = new Server(i);
  }
  Callback<void>* tasks[NumServers];

  for (int i=0; i<NumServers; i++) {
    tasks[i] = makeCallableMany(&Server::SlowAccumulate, servers[i], 1000000);
  }
  PoolType* pool = new PoolType(10);
  timer.reset();
  timer.start();
  for (int i=0; i<1000; i++) {
    pool->addTask(tasks[i%NumServers]);
  }
  pool->stop();
  timer.end();
  std::cout << "Slow Consumer:\t";
  std::cout << timer.elapsed();
  std::cout << std::endl;
  for (int i=0; i<NumServers; i++) {
    delete tasks[i];
    delete servers[i];
  }
  delete pool;
}

}  // unnamed namespace

void usage(int argc, char* argv[]) {
  std::cout << "Usage: " << argv[0] << " [1 | 2]" << std::endl;
  std::cout << "  1 is normal thread pool" << std::endl;
  std::cout << "  2 is fast thread pool" << std::endl;
  std::cout << "  defaul is to run both " << std::endl;
}

int main(int argc, char* argv[]) {
  bool all = false;
  bool num[4] = {false};
  if (argc == 1) {
    all = true;
  } else if (argc == 2) {
    std::istringstream is(argv[1]);
    int i = 0;
    is >> i;
    if (i>0 && i<=4) {
      num[i-1] = true;
    } else {
      usage(argc, argv);
      return -1;
    }
  } else {
    usage(argc, argv);
    return -1;
  }

  // no queueing of tasks really
  if (all || num[0]) {
    SlowConsumer<ThreadPoolNormal>();
  }
  if (all || num[1]) {
    SlowConsumer<ThreadPoolFast>();
  }

  // force queue building
  if (all || num[0]) {
    FastConsumer<ThreadPoolNormal>();
  }
  if (all || num[1]) {
    FastConsumer<ThreadPoolFast>();
  }

  return 0;
}
