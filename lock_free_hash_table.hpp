#ifndef MCP_LOCK_FREE_HASH_TABLE_HEADER
#define MCP_LOCK_FREE_HASH_TABLE_HEADER

#include "lock_free_list.hpp"


namespace lock_free {

using lock_free::LockFreeList; 

class LockFreeHashTable {
public:
  
  explicit LockFreeHashTable(int num_threads);

  ~LockFreeHashTable();

  bool insert(uint32_t key, uint32_t value);

  bool remove(uint32_t key);

  bool lookup(uint32_t key, uint32_t& value);

private:

  typedef LockFreeList<uint32_t, uint32_t>::Node*  node_;
  typedef node_*                                   segment_t;
  segment_t*                                       segment_table_;
  LockFreeList<uint32_t, uint32_t>*                list_;


  size_t counter_;
  size_t segment_size_;
  size_t table_size_;
  size_t buckets_size_;

  const size_t MAX_LOAD;

  static uint32_t so_regularkey(uint32_t key);
  static uint32_t so_dummykey(uint32_t key);
  static uint32_t reverse(uint32_t Num); 
  static size_t GET_PARENT(size_t index);
  void initialize_bucket(size_t index);
  node_ get_bucket(size_t index);
  void  set_bucket(size_t index, node_ node);
  //Non-copyable, non-assignable.
  LockFreeHashTable(LockFreeHashTable&);
  LockFreeHashTable& operator=(const LockFreeHashTable&);
};

} //namespace lock_free

#endif //MCP_LOCK_FREE_HASH_TABLE_HEADER
