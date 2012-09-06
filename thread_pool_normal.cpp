#include "thread_pool_normal.hpp"

namespace base {


ThreadPoolNormal::ThreadPoolNormal(int num_workers)
    : NumofWorkers(num_workers),  beStop(false) {
  TaskQueue = new queue<Callback<void>*>();
  threads = new pthread_t[num_workers];
  for (int i=0; i<num_workers; i++) {
    Callback<void>* function = makeCallableOnce(
	&ThreadPoolNormal::workerFunction, 
	this);
    threads[i] = makeThread(function);
  }
}

ThreadPoolNormal::~ThreadPoolNormal() {
  if (!beStop) {
    stop();
  }
  delete[] threads;
  while(!TaskQueue->empty()) {
    if (TaskQueue->front()->once()) {
      delete TaskQueue->front();
    }
    TaskQueue->pop();
  }
  delete TaskQueue;
}

void ThreadPoolNormal::stop() {
  {
    ScopedLock lock(&m);
    beStop = true;
    cv.signalAll();//wake up all sleeping threads, make them proceed and exit
  }
  for (int i=0; i<NumofWorkers; i++) {
    pthread_join(threads[i], NULL);
    //wait all the ongoing tasks to be finished
  }
}

void ThreadPoolNormal::addTask(Callback<void>* task) {
  ScopedLock lock(&m);
  TaskQueue->push(task);
  cv.signal();
}

int ThreadPoolNormal::count() const {
  ScopedLock lock(&m);
  return TaskQueue->size();
}

void ThreadPoolNormal::workerFunction() {  
  while(true) {
    m.lock();
    while(TaskQueue->empty() && !beStop)
      cv.wait(&m);
    if (beStop) {
      m.unlock();
      break;
    }
    Callback<void>* task = TaskQueue->front();
    TaskQueue->pop();
    m.unlock();
    (*task)();
  }
}

} // namespace base
