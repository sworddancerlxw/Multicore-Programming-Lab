#include "callback.hpp"
#include "thread.hpp"
#include "spinlock.hpp"
#include "test_unit.hpp"
#include "spinlock_mcs.hpp"

namespace {

using base::Callback;
using base::makeCallableOnce;
using base::makeThread;
using base::Spinlock;
using base::Spinlock_mcs;

// ************************************************************
// Support for concurrent test
//

class LockTester {
public:
  LockTester(Spinlock_mcs* spin, int* counter);
  ~LockTester() { }

  void start(int increments);
  void join();

  int requests() const { return requests_; }

private:
  Spinlock_mcs* spin_;
  int*      counter_;
  int       requests_;
  pthread_t tid_;

  void test(int increments);

  // Non-copyable, non-assignable
  LockTester(LockTester&);
  LockTester& operator=(LockTester&);
};

LockTester::LockTester(Spinlock_mcs* spin, int* counter)
  : spin_(spin), counter_(counter), requests_(0) {
}

void LockTester::start(int increments) {
  Callback<void>* body = makeCallableOnce(&LockTester::test, this, increments);
  tid_ = makeThread(body);
}

void LockTester::join() {
  pthread_join(tid_, NULL);
}

void LockTester::test(int increments) {
  while (increments-- > 0) {
    spin_->lock();
    ++(*counter_);
    ++requests_;
    spin_->unlock();
  }
}


// ************************************************************
// Test cases
//

TEST(Concurrency, counters) {
  Spinlock_mcs spin;
  int counter = 0;

  const int threads = 8;
  const int incs = 50000;
  LockTester* testers[threads];

  for (int i = 0; i < threads; i++) {
    testers[i] = new LockTester(&spin, &counter);
  }
  for (int i = 0; i < threads; i++) {
    testers[i]->start(incs);
  }
  for (int i = 0; i < threads; i++) {
    testers[i]->join();
    EXPECT_EQ(testers[i]->requests(), incs);
    delete testers[i];
  }

  EXPECT_EQ(counter, threads*incs);
}

}  // unnamed namespace

int main(int argc, char *argv[]) {
  return RUN_TESTS(argc, argv);
}
