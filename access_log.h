#pragma once

#include <string>

namespace logfs_fuse {

class AccessLog {
 public:
  explicit AccessLog(const std::string& log_path);
  ~AccessLog();
  void AddEntry(const std::string& access_type, const std::string& path);

 private:
  int fd_;
};

}  // namespace logfs_fuse
