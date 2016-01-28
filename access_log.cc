#include "access_log.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstring>
#include <ctime>
#include <gflags/gflags.h>
#include <glog/logging.h>

DEFINE_int64(logflush_period, 100,
             "How often (in milliseconds) to flush the log file.");

namespace logfs_fuse {

int64_t GetTimeNowMilliseconds() {
  timespec time_now;
  clock_gettime(CLOCK_MONOTONIC, &time_now);
  return (time_now.tv_sec * 1000) + (time_now.tv_nsec / 1000000);
}

AccessLog::AccessLog(const std::string& log_path) : fd_(0) {
  fd_ = open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND);
  LOG_IF(FATAL, fd_ == 0) << "Failed to open access log '" << log_path
                          << "' for write, [" << errno
                          << "] : " << strerror(errno);
  ms_of_last_flush_ = 0;
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

    int64_t ms_now = GetTimeNowMilliseconds();
    if (ms_now - ms_of_last_flush_ > FLAGS_logflush_period) {
      fsync(fd_);
      ms_of_last_flush_ = ms_now;
    }
  }
}

}  // namespace logfs_fuse
