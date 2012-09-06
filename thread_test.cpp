#include "test_unit.hpp"
#include "thread.hpp"
#include "lock.hpp"

namespace {

using base::Callback;
using base::makeCallableOnce;
using base::makeCallableMany;
using base::Mutex;
using base::ScopedLock;
//This class provides a simple method which can be wrapped as a task odject. 
Mutex m;

class Server {
public:
  Server(int initial_value) {
    value = initial_value;
    prime = false;
  }
  ~Server() {
  }

  int getValue() {
    return value;
  }

  bool getPrime() {
    return prime;
  }

  void accumulate(int limit) {
    ScopedLock lock(&m);
    for (int i=0; i<limit; i++) {
      value = value + i;
    }
  }
  
  void isPrime() {
    ScopedLock lock(&m);
    if (value == 1) {
      prime = true;
      return;
    }
    if (value%2 == 0) {
      return;
    }
    else {
      for (int i=3; i<value; i++) {
	if (value%i == 0) {
	  return;
	}
      }
    }
    prime = true;
  }

private:
  int value;
  bool prime;
};
    

TEST(Accumulate, makeThreadOnce) {
  Server my_Server(7);
  Callback<void>* body = makeCallableOnce(&Server::accumulate, &my_Server, 100);
  pthread_t tid = makeThread(body);
  pthread_join(tid, NULL);
  EXPECT_EQ(my_Server.getValue(), 4957);
}

TEST(Accumulate, makeThreadMany) {
  Server my_Server(13);
  const int NumThreads = 10;
  pthread_t threads[NumThreads];
  Callback<void>* body = makeCallableMany(&Server::accumulate, &my_Server, 50);
  for (int i=0; i<NumThreads; i++) {
    threads[i] = makeThread(body);
  }
  for (int i=0; i<NumThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  EXPECT_EQ(my_Server.getValue(), 12263);
  delete body;
}

TEST(IsPrime, makeThreadOnce) {
  Server my_Server(5);
  Callback<void>* body = makeCallableOnce(&Server::isPrime, &my_Server);
  pthread_t tid = makeThread(body);
  pthread_join(tid, NULL);
  EXPECT_TRUE(my_Server.getPrime());
}

TEST(IsPrime, makeThreadMany) {
  Server my_Server(13);
  Callback<void>* body = makeCallableMany(&Server::isPrime, &my_Server);
  pthread_t tid1 = makeThread(body);
  pthread_join(tid1, NULL);
  EXPECT_TRUE(my_Server.getPrime());
  pthread_t tid2 = makeThread(body);
  pthread_join(tid2, NULL);
  EXPECT_TRUE(my_Server.getPrime());
  delete body;
}

    

TEST(Complex, makeThreadOnce) {
  Server my_Server(5);
  Callback<void>* body1 = makeCallableOnce(&Server::accumulate, &my_Server, 90);
  pthread_t tid1 = makeThread(body1);
  pthread_join(tid1, NULL);
  Callback<void>* body2 = makeCallableOnce(&Server::isPrime, &my_Server);
  pthread_t tid2 = makeThread(body2);
  pthread_join(tid2, NULL);
  EXPECT_EQ(my_Server.getValue(), 4010);
  EXPECT_FALSE(my_Server.getPrime());
}

TEST(Complex, makeThreadMany) {
  Server my_Server(17);
  const int NumThreads = 10;
  pthread_t threads[NumThreads];
  Callback<void>* body1 = makeCallableMany(&Server::accumulate, &my_Server, 25);
  Callback<void>* body2 = makeCallableMany(&Server::isPrime, &my_Server);
  for (int i=0; i<NumThreads-1; i++) {
    threads[i] = makeThread(body1);
  }
  for (int i=0; i<NumThreads-1; i++) {
    pthread_join(threads[i], NULL);
  }
  threads[NumThreads-1] = makeThread(body2);
  EXPECT_EQ(my_Server.getValue(), 2609);
  EXPECT_TRUE(my_Server.getPrime());
  delete body1;
  delete body2;
}
} // unnammed namespace

int main(int argc, char* argv[]) {
  return RUN_TESTS(argc, argv);
}
