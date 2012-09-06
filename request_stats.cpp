#include "request_stats.hpp"

static const double ticks_per_second = base::TicksClock::ticksPerSecond();

namespace base {

//build a buffer for each thread. Every buffer records
//the requests accepted in the past one second. Each buffer
//is divided into 20 slots. Each slot represents a 0.05s time interval.
RequestStats::RequestStats(int num_threads) 
    : num_threads_(num_threads), 
      num_slots_(20) {
  req_counter_ = new slot*[num_threads_];
  for (int i=0; i<num_threads_; i++) {
    req_counter_[i] = new slot[num_slots_];
  }
}

RequestStats::~RequestStats() {
  for (int i=0; i<num_threads_; i++) {
    delete[] req_counter_[i];
    req_counter_[i] = NULL;
  }
  delete []req_counter_;
}

void RequestStats::finishedRequest(int thread_num, TicksClock::Ticks now) {
  int current_slot =((uint64_t)(now*num_slots_/ticks_per_second))%num_slots_;
  //if last update is within 0.05s, increments the number of requests.
  //else zeros the number and updates timestamp
  if ((now - req_counter_[thread_num][current_slot].last_update)*num_slots_ 
      < ticks_per_second) { 
    req_counter_[thread_num][current_slot].num_req ++;
  }
  else {
    req_counter_[thread_num][current_slot].num_req = 1;
    req_counter_[thread_num][current_slot].last_update = now;
  }
}

void RequestStats::getStats(TicksClock::Ticks now,
                            uint32_t* reqsLastSec ) const {
  //if last update occured within one second,
  //collect the number of requests into statistics.
  int req_counter = 0;
  for (int i=0; i<num_threads_; i++) {
    for (int j=0; j<num_slots_; j++) {
      if ((now - req_counter_[i][j].last_update)
	  < ticks_per_second) {
        req_counter += req_counter_[i][j].num_req;
      }
    }
  }
  *reqsLastSec = req_counter;
}

} // namespace base
