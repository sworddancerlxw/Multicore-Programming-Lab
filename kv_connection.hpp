#ifndef MCP_KV_CONNECTION_HEADER
#define MCP_KV_CONNECTION_HEADER

#include <queue>

#include "lock.hpp"
#include "connection.hpp"
#include "kv_service.hpp"
#include "http_request.hpp"
#include "http_service.hpp"
#include "http_response.hpp"

namespace kv {

using std::queue;
using base::Mutex;
using http::Request;
using http::Response;

class KVServerConnection : public base::Connection {
public:
  KVServerConnection(KVService* service, int client_fd);

private:
  Request request_;
  KVService* my_service_;
  LockFreeHashTable* lf_hashtable_;

  // base::Connection is ref counted. Use release() to
  // delete. Normally, you won't need to because the io_manager will
  // do that for you.
  virtual ~KVServerConnection() {}

  // Parses as many requests as there are in the input buffer and
  // generates responses for each of them.
  virtual bool readDone();

  bool handleRequest(Request* request);
  
  // Non-copyable, non-assignalble
  KVServerConnection(const KVServerConnection&);
  KVServerConnection& operator=(const KVServerConnection&);
};

class KVClientConnection : public base::Connection {
public:
  KVClientConnection(KVService* service);
  
  void asyncSend(Request* request, http::ResponseCallback* cb);

  void send(Request* request, Response** response);

private:
  friend class KVService;
  KVService*                      my_service_;

  KVConnectCallback*              connect_cb_;

  Mutex                           m_response_;
  queue<http::ResponseCallback*>  response_cbs_;
  // base::Connection is ref counted so the destructor shoudn't be
  // issued directly. The connection would get deleted if 'connDone()'
  // below doesn't start reading or if 'readDone()' returns false (to
  // stop reading).
  virtual ~KVClientConnection() {}

  // If the connect request went through, start reading from the
  // connection. In any case, issue the callback that was registered.
  virtual void connDone();

  // Parses as many responses as there are in the input buffer for
  // this connection and issues one callback per parsed response.
  virtual bool readDone();

  bool handleResponse(Response* response);

  void connect(const string& host, int port, KVConnectCallback* cb);

  void receiveInternal(Notification* n,
                       Response** user_response,
		       Response* new_response);

  // Non-copyable, non-assignalble
  KVClientConnection(const KVClientConnection&);
  KVClientConnection& operator=(const KVClientConnection&);
};

} // namespace kv

#endif // MCP_KV_CONNECTION_HEADER
