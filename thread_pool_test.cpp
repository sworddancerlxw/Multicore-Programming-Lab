#include "thread_pool_normal.hpp"
#include "test_unit.hpp"
#include "lock.hpp"

namespace {

using base::ThreadPool;
using base::ThreadPoolNormal;
using base::Callback;
using base::makeCallableOnce;
using base::makeCallableMany;
using base::Mutex;
using base::ScopedLock;
using base::Notification;

Mutex m;
Notification n;

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


  void accumulate(int limit) {
    ScopedLock lock(&m);
    for (int i=0; i<limit; i++) {
      value = value + i;
    }
  }
  
  void accumulateWithN(int limit) {
    ScopedLock lock(&m);
    for (int i=0; i<limit; i++) {
      value = value + i;
    }
    n.notify();
  }

private:
  int value;
};


TEST(Basic, count) {
  ThreadPoolNormal* pool = new ThreadPoolNormal(5);
  EXPECT_EQ(pool->count(), 0);
  pool->stop();
  delete pool;
}

TEST(Basic, addTask1) {
  Server my_Server(20);
  Callback<void>* task1 = makeCallableOnce(&Server::accumulate, &my_Server, 20);
  Callback<void>* task2 = makeCallableOnce(&Server::accumulate, &my_Server, 20);
  ThreadPoolNormal* pool = new ThreadPoolNormal(0);
  pool->addTask(task1);
  EXPECT_EQ(pool->count(), 1);
  pool->addTask(task2);
  EXPECT_EQ(pool->count(), 2);
  pool->stop();
  delete pool;
}

TEST(Basic, addTask2) {
  Server my_Server(50);
  Callback<void>* task1 = makeCallableMany(&Server::accumulate, &my_Server, 50);
  Callback<void>* task2 = makeCallableMany(&Server::accumulate, &my_Server, 50);
  ThreadPoolNormal* pool = new ThreadPoolNormal(0);
  pool->addTask(task1);
  EXPECT_EQ(pool->count(), 1);
  pool->addTask(task2);
  EXPECT_EQ(pool->count(), 2);
  pool->stop();
  delete pool;
  delete task1;
  delete task2;
}

TEST(Basic, stop1) {
  Server my_Server(20);
  Callback<void>* task1 = makeCallableOnce(&Server::accumulateWithN, &my_Server, 20);
  Callback<void>* task2 = makeCallableOnce(&Server::accumulate, &my_Server, 20);
  ThreadPoolNormal* pool = new ThreadPoolNormal(5);
  pool->addTask(task1);
  n.wait();
  n.reset();
  pool->stop();
  pool->addTask(task2);
  EXPECT_EQ(pool->count(), 1);
  delete pool;
}


TEST(Basic, stop2) {
  Server my_Server(20);
  Callback<void>* task1 = makeCallableMany(&Server::accumulate, &my_Server, 20);
  Callback<void>* task2 = makeCallableMany(&Server::accumulateWithN, &my_Server, 20);
  ThreadPoolNormal* pool = new ThreadPoolNormal(5);
  pool->addTask(task1);
  pool->addTask(task2);
  n.wait();
  n.reset();
  pool->stop();
  EXPECT_EQ(pool->count(), 0);
  delete pool;
  delete task1;
  delete task2;
  
}

TEST(Running, SingleThread) {
  Server my_Server(73);
  Callback<void>* task1 = makeCallableOnce(&Server::accumulate, &my_Server, 100);
  Callback<void>* task2 = makeCallableOnce(&Server::accumulateWithN, &my_Server, 100);
  ThreadPoolNormal* pool = new ThreadPoolNormal(1);
  pool->addTask(task1);
  pool->addTask(task2);
  n.wait();
  n.reset();
  pool->stop();
  EXPECT_EQ(my_Server.getValue(), 9973);
  EXPECT_EQ(pool->count(), 0);
  delete pool;
}

TEST(Running, MultiThreaded) {
  Server my_Server(91);
  Callback<void>* task1 = makeCallableMany(&Server::accumulate, &my_Server, 10);
  Callback<void>* task2 = makeCallableMany(&Server::accumulate, &my_Server, 10);
  Callback<void>* task3 = makeCallableMany(&Server::accumulateWithN, &my_Server, 10);
  ThreadPoolNormal* pool = new ThreadPoolNormal(10);
  for (int i=0; i<100; i++) {
    pool->addTask(task1);
    pool->addTask(task2);
  }
  pool->addTask(task3);
  n.wait();
  n.reset();
  pool->stop();
  EXPECT_EQ(my_Server.getValue(), 9136);
  EXPECT_EQ(pool->count(), 0);
  for (int i=0; i<5; i++) {
    pool->addTask(task1);
    pool->addTask(task2);
  }
  EXPECT_EQ(pool->count(), 10);
  EXPECT_EQ(my_Server.getValue(), 9136);
  delete pool;
  delete task1;
  delete task2;
  delete task3;
  
}

//The following tests take the thread pool itself as server object.
TEST(Running, stopAsTask) {
  ThreadPoolNormal* pool = new ThreadPoolNormal(5);
  Server my_Server(10);
  Callback<void>* task1 = makeCallableMany(&Server::accumulate, &my_Server, 20);
  Callback<void>* task2 = makeCallableMany(&ThreadPoolNormal::stop, pool);
  Callback<void>* task3 = makeCallableMany(&Server::accumulateWithN, &my_Server, 20);
  pool->addTask(task1);
  pool->addTask(task3);
  n.wait();
  n.reset();
  pool->addTask(task2);//task2 issues a stop to the task queue.
  sleep(1);
  EXPECT_EQ(my_Server.getValue(), 390);
  EXPECT_EQ(pool->count(), 0);
  delete pool;
  delete task1;
  delete task2;
  delete task3;
}

TEST(Running, addTaskAsTask) {
  ThreadPoolNormal* pool = new ThreadPoolNormal(10);
  Server my_Server(20);
  Callback<void>* body1 = makeCallableMany(&Server::accumulate, &my_Server, 20);
  Callback<void>* body2 = makeCallableMany(&Server::accumulateWithN, &my_Server, 20);
  Callback<void>* task1 = makeCallableMany(&ThreadPoolNormal::addTask, pool, body1);
  Callback<void>* task2 = makeCallableMany(&ThreadPoolNormal::stop, pool);
  Callback<void>* task3 = makeCallableMany(&ThreadPoolNormal::addTask, pool, body2);
  for (int i=0; i<10; i++) {
    pool->addTask(task1);//The executing of this task adds a task to the task queue.
  }
  pool->addTask(task3);
  n.wait();
  n.reset();
  pool->addTask(task2);
  EXPECT_EQ(my_Server.getValue(), 2110);
  for (int i=0; i<10; i++) {
    pool->addTask(task1);
  }
  EXPECT_EQ(pool->count(), 10);
  EXPECT_EQ(my_Server.getValue(), 2110);
  delete pool;
  delete task1;
  delete task2;
  delete task3;
  delete body1;
  delete body2;
}
} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
