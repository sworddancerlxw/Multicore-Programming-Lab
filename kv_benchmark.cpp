#include <iomanip>

#include "callback.hpp"
#include "signal_handler.hpp"
#include "kv_connection.hpp"
#include "http_response.hpp"
#include "kv_service.hpp"
#include "io_manager.hpp"

// client --local 15000 --server 10.0.1.12 --port 15001 --num_cons 500
// --rate 500 --num-calls 10

using base::AcceptCallback;
using base::Callback;
using base::IOManager;
using base::ServiceManager;
using base::makeCallableOnce;
using base::makeCallableMany;
using kv::KVConnectCallback;
using kv::KVClientConnection;
using kv::KVService;
using http::Request;
using http::Response;
using http::ResponseCallback;

using namespace std;

class Client {
public:
  Client();
  ~Client();

  void start(KVClientConnection* conn);
  void doRequest();
  void requestDone(Response* response);

  uint32_t counter;
private:
  KVClientConnection*   conn_;
  Callback<void>*       request_cb_;   // owned here
  ResponseCallback*     response_cb_;  // owned here

  // non-copyable, non-assignable
  Client(const Client&);
  Client& operator=(const Client&);
};



Client::Client() : counter(0), conn_(NULL), request_cb_(NULL), 
                   response_cb_(NULL) { }

Client::~Client() {
  delete request_cb_;
  delete response_cb_;
}

void Client::start(KVClientConnection* conn) {
  if (! conn->ok()) {
    LOG(LogMessage::ERROR) << "Cannot connect: " << conn->errorString();
    return;
  }

  conn_ = conn;
  request_cb_ = makeCallableMany(&Client::doRequest, this);
  response_cb_ = makeCallableMany(&Client::requestDone, this);

  // Issue the first request. acquire() here is necessary because
  // doRequest() always release()s. And that makes it possible for it
  // to be scheduled using a timer.
  conn_->acquire();
  doRequest();
}

void Client::doRequest() {
  if (! conn_->ok()) {
    LOG(LogMessage::ERROR) << "In new request: " << conn_->errorString();

  } else {
    http::Request request;
    request.method = "GET";
    request.address = "/12";
    request.version = "KV/1.1";
    conn_->asyncSend(&request, response_cb_);
  }
  conn_->release();
}

void Client::requestDone(Response* response) {
  if (! conn_->ok()) {
    //LOG(LogMessage::ERROR) << "In requestDone: " << conn_->errorString();
    return;
  }

  conn_->acquire();
  IOManager* io_manager = conn_->io_manager();
  counter++;
  if (!io_manager->stopped()) {
    io_manager->addTask(request_cb_);
  }
}


void runBenchmark(const int num_clients, const int num_workers) {

  ServiceManager service(num_workers);
  KVService kv_service(15000, &service);

  IOManager* io_manager = service.io_manager();
  int total_counter = 0;

  for (uint32_t i=0; i<1000; i++) {
    kv_service.lf_hashtable()->insert(i,i);
  }
  Client clients[num_clients];
  for (int i=0; i<num_clients; ++i) {
    KVConnectCallback* connect_cb = makeCallableOnce(&Client::start, &clients[i]);
    kv_service.asyncConnect("127.0.0.1", 15000, connect_cb);
  }

  Callback<void>* terminate = makeCallableOnce(&ServiceManager::stop, &service);
  io_manager->addTimer(1.0, terminate);
  // Launch a progress meter in the background. If things hang, let
  // the meter kill this process.

  service.run();
  for (int i=0; i<num_clients; i++) {
    total_counter += clients[i].counter;
  }

  cout << setiosflags(ios::left) << setw(15) << num_clients;
  cout << setiosflags(ios::left) << setw(15) << num_workers;
  cout << total_counter << endl;
}


int main(int argc, char* argv[]) {
  cout << setiosflags(ios::left) << setw(15) << "# of clients";
  cout << setiosflags(ios::left) << setw(15) << "# of workers";
  cout << setiosflags(ios::left) << "# of responses per second";
  cout << endl;
  for (int i=0; i<5; i++) {
    for (int j=0; j<4; j++) {
      runBenchmark(16<<i, 4<<j);
    }
  }
  return 0;
}
