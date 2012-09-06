#ifndef MCP_BASE_THREAD_POOL_NORMAL_HEADER
#define MCP_BASE_THREAD_POOL_NORMAL_HEADER

#include "callback.hpp"
#include "thread_pool.hpp"
#include <queue>

namespace base {

using namespace std;

class ThreadPoolNormal : public ThreadPool {
public:
  // ThreadPoolNormal interface
  explicit ThreadPoolNormal(int num_workers);
  virtual ~ThreadPoolNormal();

  virtual void addTask(Callback<void>* task);
  virtual void stop();
  virtual int count() const;

private:
  queue<Callback<void>*>* TaskQueue;
  pthread_t* threads;
  int NumofWorkers;
  bool beStop;
  ConditionVar cv;
  mutable Mutex m;

  //the workerFunction is what every worker thread need to execute:
  //visit the task queue, acquire a task and run it.
  void workerFunction();
    
  // Non-copyable, non-assignable.
  ThreadPoolNormal(const ThreadPoolNormal&);
  ThreadPoolNormal& operator=(const ThreadPoolNormal&);
};

} // namespace base

#endif // MCP_BASE_THREAD_POOL_NORMAL_HEADER
