#include "access_log.h"

#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glog/logging.h>

namespace logfs_fuse {

AccessLog::AccessLog(const std::string& log_path) : fd_(0) {
  fd_ = open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  LOG_IF(FATAL, fd_ == 0) << "Failed to open access log '" << log_path
                          << "' for write, [" << errno
                          << "] : " << strerror(errno);
}

AccessLog::~AccessLog() {
  if (fd_) {
    close(fd_);
  }
}

void AccessLog::AddEntry(const std::string& access_type,
                         const std::string& log_path) {
  if (fd_) {
    write(fd_, access_type.c_str(), access_type.size());
    write(fd_, " : ", 2);
    write(fd_, log_path.c_str(), log_path.size());
    write(fd_, "\n", 1);
    fsync(fd_);
  }
}

}  // namespace logfs_fuse
