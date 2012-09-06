#ifndef MCP_KV_SERVICE_HEADER
#define MCP_KV_SERVICE_HEADER

#include "service_manager.hpp"
#include "lock_free_hash_table.hpp"
#include "request_stats.hpp"
#include "lock.hpp"

namespace kv {

using std::string;
using base::AcceptCallback;
using base::Notification;
using base::RequestStats;
using base::ServiceManager;
using lock_free::LockFreeHashTable;

class KVClientConnection;

typedef base::Callback<void, KVClientConnection*> KVConnectCallback;

class KVService {
public:
  KVService(int port, ServiceManager* service_manager);
  ~KVService();

  void stop();
 
  void asyncConnect(const string& host, int post, KVConnectCallback* cb);

  void connect(const string& host, int port, KVClientConnection** conn); 
  // accessors

  ServiceManager* service_manager() { return service_manager_; }
  LockFreeHashTable* lf_hashtable() { return &lf_hashtable_; }
  RequestStats* stats() { return &stats_; }

private:
  ServiceManager* service_manager_;  // not owned here
  RequestStats stats_;
  LockFreeHashTable lf_hashtable_;

  void acceptConnection(int clinet_fd);

  void connectInternal(Notification* n,
                       KVClientConnection** user_conn,
		       KVClientConnection* new_conn);

  // Non-copyable, non-assignable.
  KVService(const KVService&);
  KVService& operator=(const KVService&);
};

} // namespace kv

#endif //  MCP_KV_SERVICE_HEADER
