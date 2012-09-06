#include <iostream>
#include <iomanip>
#include "lock.hpp"
#include "spinlock.hpp"
#include "timer.hpp"
#include "callback.hpp"
#include "thread.hpp"
#include "spinlock_mcs.hpp"

namespace {

using base::Mutex;
using base::Callback;
using base::makeThread;
using base::makeCallableOnce;
using base::Spinlock;
using base::Spinlock_mcs;
using base::Timer;
using namespace std;
	
template <typename LockType>
class LockTester {
public:
  LockTester( int num_threads)
    :  requests_(0), num_threads_(num_threads) {
    threads = new pthread_t[num_threads_];
  }
		
  ~LockTester() {
     delete[] threads;
  }
		
  void start(int increments, int increments_step) {
    for (int i=0; i<num_threads_; i++) {
      Callback<void>* body = 
	makeCallableOnce(&LockTester::test, this, increments, increments_step);
	threads[i] = makeThread(body);
     }
   }
		
   void join() {
     for (int i=0; i<num_threads_; i++) {
       pthread_join(threads[i], NULL);
     }
   }
		
   int requests() const { return requests_; }
		
private:
  LockType lock_;
  int requests_, num_threads_;
  pthread_t* threads;
		
  void test(int increments, int increments_step) {
    while (increments > 0) {
      lock_.lock();
      for (int i=0; i<increments_step; i++) {
	increments -= 1;
	requests_ += 1;
      }
      lock_.unlock();
    }
  }
		
  //Non-copyable, non-assignable
  LockTester(LockTester&);
  LockTester& operator=(LockTester&);
};
	
}

//*************************************
//Benchmarks
//


template <typename T>
void Increments(int num_threads, int incs, int incstep) {	
  Timer timer;
  LockTester<T> tester(num_threads);
	
  timer.reset();
  timer.start();
	
  tester.start(incs, incstep);
  tester.join();
	
  timer.end();
	
  if (tester.requests() != incs*num_threads) {
    cout << "All the tasks are not finished." << endl;
    return;
  }
  
  cout << setiosflags(ios::left) 
       << setw(15) << 1/timer.elapsed()*num_threads;
}



int main(int argc, char *argv[]) {
  //In this benchmark, I test the throughput in three situations under two types of lock.
  //Three situations: between each lock/unlock, 1/10/200 loops run, use incstep to represent it.
  //Theoretically, when incstep is small, spinning lock should be more efficient,
  //when incstep is large, mutual exclusion should be more efficient.
  //My benchmark results show some agreement with this.
	
  for (int j=0; j<3; j++) {
    int incstep;
    const int NUM_CORES = sysconf( _SC_NPROCESSORS_ONLN );
    if (j == 0)      {incstep = 1;}
    else if (j == 1) {incstep = 10;}
    else             {incstep = 200;}
    cout << "-------Throughput under three types of locks: tasks/second" ;
    cout << "( "<< incstep << " loops in critial region)-------" << endl;
    cout << setiosflags(ios::left) << setw(15) << "# of Threads";
    cout << setiosflags(ios::left) << setw(15) << "Mutex";
    cout << setiosflags(ios::left) << setw(15) << "Spin";
    cout << setiosflags(ios::left) << setw(15) << "Spin_mcs";
    cout << endl;
		
    for (int i=1; i<=NUM_CORES; i++) {
      cout << setiosflags(ios::left) << setw(15) << i; 
      Increments<Mutex>(i, 100000, incstep);
      Increments<Spinlock>(i, 100000, incstep);
      Increments<Spinlock_mcs>(i, 100000, incstep);
      cout << endl;
    }
  } 
	
	
}

