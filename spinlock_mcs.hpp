#ifndef MCP_BASE_SPINLOCK_MCS_HEADER
#define MCP_BASE_SPINLOCK_MCS_HEADER

#include "lock.hpp"
#include "thread_local.hpp"

namespace base {


class Spinlock_mcs : public Lock{
public:
  Spinlock_mcs(): tail_(NULL) {}

  ~Spinlock_mcs() {}

  void lock() {
    Node* newnode = local_.getAddr();
    newnode->next = NULL;

    Node* pred = __sync_lock_test_and_set(&tail_, newnode);
    if (pred != NULL) {
      newnode->locked = true;
      pred->next = newnode;
      while (newnode->locked) {}
    }
  }

  void unlock() {
    Node* newnode = local_.getAddr();
    if (newnode->next == NULL) {
      if (__sync_bool_compare_and_swap(&tail_, newnode, NULL)) {
	return;
      }
      while(newnode->next == NULL) {}
    }
    newnode->next->locked = false;
  }
  
private:
  struct Node {
    volatile Node* next;
    volatile bool locked;
    Node(): next(NULL), locked(false) {}
  };

  ThreadLocal<Node> local_;
  struct Node* tail_;

  // Non-copyable, non-assignable
  Spinlock_mcs(Spinlock_mcs&);
  Spinlock_mcs operator=(Spinlock_mcs&);
};

}
#endif
