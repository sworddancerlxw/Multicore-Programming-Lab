#include "ticks_clock.hpp"
#include "test_unit.hpp"
#include "thread_pool.hpp"
#include "thread_pool_fast.hpp"
#include "callback.hpp"

#include "request_stats.hpp"

using base::TicksClock;
using base::ThreadPool;
using base::ThreadPoolFast;
using base::makeCallableMany;
using base::Callback;
using base::RequestStats;

namespace {

TEST(SingleThread, NoRequests) {
  const int num_threads = 1;
  uint32_t reqsLastSec = 1;
  RequestStats stat(num_threads);

  stat.getStats(TicksClock::getTicks(), &reqsLastSec);
  
  EXPECT_EQ(reqsLastSec, 0);
}

TEST(SingleThread, ManyRequests) {
  const int num_threads = 1;
  uint32_t reqsLastSec = 0;
  RequestStats stat(num_threads);
  
  for (int i=0; i<200; i++) {
    stat.finishedRequest(0, TicksClock::getTicks());
  }
  stat.getStats(TicksClock::getTicks(), &reqsLastSec);

  EXPECT_EQ(reqsLastSec, 200);
}

TEST(SingleThread, RequestsWithSleep) {
  const int num_threads = 1;
  uint32_t reqsLastSec = 0;
  RequestStats stat(num_threads);
  
  for (int i=0; i<200; i++) {
    stat.finishedRequest(0, TicksClock::getTicks());
  }
  sleep(2);
 
  stat.getStats(TicksClock::getTicks(), &reqsLastSec);

  EXPECT_EQ(reqsLastSec, 0);
}

TEST(MultiThread, ManyRequests) {
  const int num_threads = 10;
  uint32_t reqsLastSec= 0;
  RequestStats stat(num_threads);
  ThreadPool* pool = new ThreadPoolFast(num_threads);

  Callback<void>* finished = makeCallableMany(&RequestStats::finishedRequest,
                                         &stat, ThreadPoolFast::ME(),
					 TicksClock::getTicks()); 
 
  for (int i=0; i<20000; i++) {
    pool->addTask(finished);
  }

  sleep(0.5);
  pool->stop();

  stat.getStats(TicksClock::getTicks(), &reqsLastSec);

  EXPECT_EQ(reqsLastSec, 20000);
  delete pool;
  delete finished;
}

}  // unamed namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc, argv);
}
