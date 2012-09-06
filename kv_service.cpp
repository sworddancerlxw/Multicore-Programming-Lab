#include <errno.h>
#include <string.h>

#include "kv_service.hpp"
#include "kv_connection.hpp"

namespace kv {

using base::makeCallableMany;

KVService::KVService(int port, ServiceManager* service_manager)
  : service_manager_(service_manager),
    stats_(service_manager->num_workers()),
    lf_hashtable_(service_manager->num_workers()) {
  AcceptCallback* cb = makeCallableMany(&KVService::acceptConnection, this);
  service_manager_->registerAcceptor(port, cb);
}

KVService::~KVService() {
}

void KVService::stop() {
  service_manager_->stop();
}

void KVService::acceptConnection(int client_fd) {
  if (service_manager_->stopped()) {
    return;
  }

  if (client_fd < 0) {
    LOG(LogMessage::ERROR) << "Error accepting: " << strerror(errno);
    service_manager_->stop();
    return;
  }

  new KVServerConnection(this, client_fd);
}

void KVService::asyncConnect(const string& host,
                             int port,
			     KVConnectCallback* cb) {
  if (service_manager_->stopped()) {
    return;
  }
  
  KVClientConnection* conn = new KVClientConnection(this);
  conn->connect(host, port, cb);
}

void KVService::connect(const string& host,
                        int port,
			KVClientConnection** conn) {
  Notification n;
  KVConnectCallback* cb = makeCallableOnce(&KVService::connectInternal,
                                            this,
					    &n,
					    conn);
  asyncConnect(host, port, cb);
  n.wait();
}

void KVService::connectInternal(Notification* n,
                                KVClientConnection** user_conn,
				KVClientConnection* new_conn) {
  *user_conn = new_conn;
  n->notify();
}

} // namespace kv
