#pragma once

#include <cstdint>
#include <string>

namespace logfs_fuse {

class AccessLog {
 public:
  explicit AccessLog(const std::string& log_path);
  ~AccessLog();
  void AddEntry(const std::string& access_type, const std::string& path);

 private:
  int fd_;
  int64_t ms_of_last_flush_;
};

}  // namespace logfs_fuse
