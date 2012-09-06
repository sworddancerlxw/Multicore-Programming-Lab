#include <sstream>

#include "http_parser.hpp"
#include "kv_connection.hpp"

namespace kv {

using std::istringstream;
using std::ostringstream;  
using base::TicksClock;
using base::RequestStats;
using base::ThreadPoolFast;
using base::Connection;
using base::Buffer;
using lock_free::LockFreeHashTable;
using http::Parser;
using http::Response;

KVServerConnection::KVServerConnection(KVService* service, int client_fd)
  : Connection(service->service_manager()->io_manager(), client_fd),
    my_service_(service), 
    lf_hashtable_(service->lf_hashtable()) {
  startRead();
}

bool KVServerConnection::readDone() {
  int rc;

  while(true) {
    request_.clear();
    Buffer::Iterator it = in_.begin();
    rc = Parser::parseRequest(&it, &request_);
    if (rc < 0) {
      LOG(LogMessage::ERROR) << "Error parsing request";
      return false;
    } else if (rc > 0) {
      return true;
    } else {
      in_.consume(it.bytesRead());
      if (! handleRequest(&request_)) {
	return false;
      }
      if (it.eob()) {
	return true;
      }
    }
  }
}

bool KVServerConnection::handleRequest(Request* requst) {
  if (request_.address == "quit") {
    LOG(LogMessage::NORMAL) << "Server stop requested!";
    my_service_->stop();
    return false;
  }

  RequestStats* stats = my_service_->stats();
  if (request_.address == "stats") {
    uint32_t reqsLastSec;
    stats->getStats(TicksClock::getTicks(), &reqsLastSec);
    ostringstream stats_stream;
    stats_stream << reqsLastSec;
    string stats_string = stats_stream.str();

    m_write_.lock();
    out_.write("HTTP/1.1 200 OK\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: Lab02a\r\n");
    out_.write("Accept-Ranges: bytes\r\n");

    ostringstream os;
    os << "Content-Length: " << stats_string.size() << "\r\n";
    out_.write(os.str().c_str());
    out_.write("Content-Type: text/html\r\n");
    out_.write("\r\n");
 
    out_.write(stats_string.c_str());

    m_write_.unlock();

    startWrite();
    return true;
  }

  uint32_t key;
  istringstream key_stream(request_.address);
  key_stream >> key;
  uint32_t value = 0;
  if (lf_hashtable_->lookup(key, value)) {
    m_write_.lock();

    ostringstream value_stream;
    value_stream << value;
    out_.write("HTTP/1.1 200 OK\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: Lab02a\r\n");
    out_.write("Accept-Ranges: bytes\r\n");

    ostringstream os;
    os << "Content-Length: " << value_stream.str().size() << "\r\n";
    out_.write(os.str().c_str());
    out_.write("Content-Type: text/html\r\n");
    out_.write("\r\n");

    out_.write(value_stream.str().c_str());
 
    m_write_.unlock();

  } else {

    m_write_.lock();
  
    out_.write("HTTP/1.1 200 OK\r\n");
    out_.write("Date: Wed, 28 Oct 2009 15:24:11 GMT\r\n");
    out_.write("Server: Lab02a\r\n");
    out_.write("Accept-Ranges: bytes\r\n");

    out_.write("Content-Length: 0\r\n");
    out_.write("Content-Type: text/html\r\n");
    out_.write("\r\n");
    out_.write("value corresponding to the key not found\r\n");
    
    m_write_.unlock();
  }

  stats->finishedRequest(ThreadPoolFast::ME(), TicksClock::getTicks());

  startWrite();
  return true;
}

KVClientConnection::KVClientConnection(KVService* service)
  : Connection(service->service_manager()->io_manager()),
    my_service_(service) { }

void KVClientConnection::connect(const string& host,
                                 int port,
				 KVConnectCallback* cb) {
  connect_cb_ = cb;
  startConnect(host, port);
}

void KVClientConnection::connDone() {
  if (ok()) {
    startRead();
  }

  (*connect_cb_)(this);
}

bool KVClientConnection::readDone() {
  int rc;
  while (true) {
    Buffer::Iterator it = in_.begin();
    Response* response = new Response();
    rc = Parser::parseResponse(&it, response);
    if (rc < 0) {
      delete response;
      LOG(LogMessage::ERROR) << "Error parsing response";
      return false;
    } else if (rc > 0) {
      delete response;
      return true;
    } else {
      in_.consume(it.bytesRead());
      if (! handleResponse(response)) {
	return false;
      }
      if (it.eob()) {
	return true;
      }
    }
  }
}

bool KVClientConnection::handleResponse(Response* response) {
  http::ResponseCallback* cb = NULL;
  m_response_.lock();
  if (! response_cbs_.empty()) {
    cb = response_cbs_.front();
    response_cbs_.pop();
  }
  m_response_.unlock();

  if (cb != NULL) {
    (*cb)(response);
  }
  return true;
}

void KVClientConnection::asyncSend(Request* request, 
                                   http::ResponseCallback* cb) {
  m_response_.lock();
  response_cbs_.push(cb);
  
  m_write_.lock();
  request->toBuffer(&out_);
  m_write_.unlock();

  m_response_.unlock();

  startWrite();  
}

void KVClientConnection::send(Request* request, Response** response) {
  Notification* n = new Notification();
  http::ResponseCallback* cb 
      = makeCallableOnce(&KVClientConnection::receiveInternal, this, 
	                 n, response);
  asyncSend(request, cb);
  n->wait();
  delete n;
}

void KVClientConnection::receiveInternal(Notification* n,
                                         Response** user_response,
					 Response* new_response) {
  *user_response = new_response;
  n->notify();
}

}  // namespace kv
