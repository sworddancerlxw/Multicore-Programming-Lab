#include "file_cache.hpp"
#include "logging.hpp"


namespace base {

FileCache::FileCache(int max_size) : max_size_(max_size),
  bytes_used_(0), num_pins_(0), num_hits_(0), num_failed_(0) {}

// REQUIRES: No ongoing pin. This code assumes no one is using the
// cache anymore
FileCache::~FileCache() {
  ScopedWLock wlock(&rwm_);
    for (CacheMap::iterator it = cache_map_.begin();
	it != cache_map_.end(); it++) {
      delete(it->second->buf);
      delete(it->second);
    }
}

FileCache::CacheHandle FileCache::pin(const string& file_name,
                                      Buffer** buf,
                                      int* error) {
  __sync_fetch_and_add(&num_pins_, 1);
  CacheHandle h = 0;
  
  rwm_.rLock();  
  CacheMap::iterator it = cache_map_.find(&file_name);
  // if the file to be pinned is in the cache
  if (it != cache_map_.end()) {
    __sync_fetch_and_add(&num_hits_, 1);
    __sync_fetch_and_add(&it->second->stat, 1);
    h = it->first;
    *buf = it->second->buf;
    rwm_.unlock();
  }
  //the file to be pinned is not in the cache
  else {
    rwm_.unlock();
    size_t file_size = getFileSize(file_name);
    Buffer* new_buf = loadFileToBuffer(file_name);
    if (new_buf != NULL) {
      Node* new_node = new Node(file_name, new_buf);
      {
	ScopedWLock wLock(&rwm_);
	//repeatedly evict unpinned pages until the space is got or there is no more
	//unpinned buffers in the cache
	while( bytes_used_ + file_size > max_size_) {	  
	  CacheMap::iterator tmpit;
	  //find the first unpinned buffer 
	  for (tmpit = cache_map_.begin();
	       tmpit != cache_map_.end(); tmpit++) {
	    if (tmpit->second->stat == 0) {
	      break;
	    }
	  }
	  if (tmpit != cache_map_.end()) {
	    bytes_used_ -= getFileSize(tmpit->second->file_name);
	    delete tmpit->second->buf;
	    delete tmpit->second;
	    cache_map_.erase(tmpit);
	  }
	  else {
	    break;//no more unpinned buffer in the cache, break loop
	  }
	}
	if (bytes_used_ + file_size <= max_size_) {//if enought space is made, insert new file
          std::pair<CacheMap::iterator, bool> it;
	  it = cache_map_.insert(make_pair(&new_node->file_name, new_node));
	  if (it.second) {
	    h = reinterpret_cast<CacheHandle>(&new_node->file_name);
	    *buf = new_node->buf;
	    bytes_used_ += file_size;
	  }
	  else {
	    LOG(LogMessage::ERROR) << "can't pin file " << new_node->file_name;
	  }
        }
	else {//Space not big enough for pin, fail
	  __sync_fetch_and_add(&num_failed_, 1);
	  *buf = 0;
	}
      }
    }
  }
  return h;
}

void FileCache::unpin(CacheHandle h) {
  ScopedRLock rLock(&rwm_);
  const string* file_ptr = reinterpret_cast<const string*>(h);
  CacheMap::iterator it = cache_map_.find(file_ptr); 
  if (it != cache_map_.end()) {
    if (it->second->stat != 0) {
      __sync_fetch_and_sub(&it->second->stat, 1);
    }
    else {
      LOG(LogMessage::FATAL) << "can't unpin file "<< *file_ptr;
    }
  }
}

//this function get the size of a file
size_t FileCache::getFileSize(const string& file_name) {
  int fd = ::open(file_name.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG(LogMessage::WARNING) << "could not open" << file_name
                             << ": " << strerror(errno);
    return 0;
  }

  struct stat stat_buf;
  fstat(fd, &stat_buf);
  return stat_buf.st_size;
}

//this function load a file into a buffer
Buffer* FileCache::loadFileToBuffer(const string& file_name) {
  int fd = ::open(file_name.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG(LogMessage::WARNING) << "could not open" << file_name
                             << ": " << strerror(errno);
    return NULL;
  }
  
  struct stat stat_buf;
  fstat(fd, &stat_buf);

  size_t to_read = stat_buf.st_size;
  Buffer* buf = new Buffer();
  while(to_read > 0) {
    buf->reserve(buf->BlockSize);
    int bytes_read = read(fd, buf->writePtr(), buf->writeSize());

    if (bytes_read < 0) {
      LOG(LogMessage::ERROR) << "could not read file " <<file_name
	                     << ": " << strerror(errno);
      close(fd);
      delete buf;
      return NULL;
    }

    to_read -= bytes_read;
  }

  if (to_read != 0) {
    LOG(LogMessage::WARNING) << "file change while reading " << file_name;
  }
  buf->advance(stat_buf.st_size - to_read);
  close(fd);
  return buf;
}
}  // namespace base
