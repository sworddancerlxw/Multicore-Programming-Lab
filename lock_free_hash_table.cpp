#include "lock_free_hash_table.hpp"
#include <cmath>

namespace lock_free {

using lock_free::LockFreeList;


LockFreeHashTable::LockFreeHashTable(int num_threads)
  : segment_table_(NULL), counter_(0), segment_size_(10000),
    table_size_(10000), buckets_size_(10000), MAX_LOAD(10) {
  segment_table_ = new segment_t[table_size_];
  for (size_t i=0; i<table_size_; i++) {
    segment_table_[i] = NULL;
  }
  list_= new LockFreeList<uint32_t, uint32_t>(num_threads);
}

LockFreeHashTable::~LockFreeHashTable() {
  delete list_;
  for (size_t i=0; i<table_size_; i++) {
    if (segment_table_[i] != NULL) {
      delete segment_table_[i];
    }
  }
  delete segment_table_;
}

bool LockFreeHashTable::insert(uint32_t key, uint32_t value) {
  size_t index = key % buckets_size_;
  if (get_bucket(index) == NULL) {
    initialize_bucket(index);
  }
  if ((list_->insert(get_bucket(index), so_regularkey(key), value, false)) == NULL) {
    return false;
  }
  size_t csize = buckets_size_;
  if (__sync_add_and_fetch(&counter_, 1)/csize > MAX_LOAD) {
    __sync_bool_compare_and_swap(&buckets_size_, csize, 2*csize);
  }
  //std::cout << key << std::endl;
  return true;
}


bool LockFreeHashTable::remove(uint32_t key) {
  size_t index = key % table_size_;
  if (get_bucket(index) == NULL) {
    initialize_bucket(index);
  }
  if (!list_->remove(get_bucket(index), so_regularkey(key))) {
    return false;
  }
  __sync_sub_and_fetch(&counter_, 1);
  return true;
}


bool LockFreeHashTable::lookup(uint32_t key, uint32_t& value) {
  size_t index = key % table_size_;
  if (get_bucket(index) == NULL) {
    initialize_bucket(index);
  }
  return (list_->lookup(get_bucket(index), so_regularkey(key), value));
}


uint32_t LockFreeHashTable::so_regularkey(uint32_t key_t) {
  return reverse(key_t|0x80000000);
}



uint32_t LockFreeHashTable::so_dummykey(uint32_t key_t) {
  return reverse(key_t);
}

uint32_t LockFreeHashTable::reverse(uint32_t Num) {
  uint32_t ret = 0;
  for (int i=0; i<32; i++) {
    ret <<= 1;
    ret|= Num & 1;
    Num >>= 1;
  }
  return ret;
}

size_t LockFreeHashTable::GET_PARENT(size_t index) {
  int count = 0;
  size_t temp = index;
  while ((temp >> 1) != 0) {
    temp >>= 1;
    count++;
  }
  temp = pow(2, count) - 1;
  index = index & temp;
  return index;
}

void LockFreeHashTable::initialize_bucket(size_t index) {
  if (index != 0) {
    size_t parent = GET_PARENT(index);  
    if (get_bucket(parent) == NULL) {
      initialize_bucket(parent);
    }
    set_bucket(index, list_->insert(get_bucket(parent), so_dummykey(index), NULL, true));
  }
  else {
    set_bucket(index, list_->insert(get_bucket(index), so_dummykey(index), NULL, true));
  }
}

LockFreeHashTable::node_ LockFreeHashTable::get_bucket(size_t index) {
  size_t segment = index/segment_size_;
  if (segment_table_[segment] == NULL) {
    return NULL;
  }
  return segment_table_[segment][index % segment_size_];
}

void LockFreeHashTable::set_bucket(size_t index, node_ node) {
  size_t segment = index/segment_size_;
  segment_t new_segment;
  if (segment_table_[segment] == NULL) {
    new_segment = new node_[segment_size_];
    for (size_t i=0; i<segment_size_; i++) {
      new_segment[i] = NULL;
    }
  
    if (!__sync_bool_compare_and_swap(&segment_table_[segment], NULL, new_segment)) {
      delete new_segment;
    }
  }
  segment_table_[segment][index % segment_size_] = node;
}

} //namespace lock_free
