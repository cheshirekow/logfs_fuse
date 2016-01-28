#pragma once

#include <string>
#include "fuse_include.h"

namespace logfs_fuse {

class AccessLog;
class FuseContext;

/// encapsulates the path to a mount point, the fuse channel, and fuse object
/// for the fuse filesystem mounted at that point
class MountPoint {
 private:
  std::string mount_point_;  ///< path to the mount point
  std::string real_tree_;    ///< path to the real tree we are mirroring
  std::string log_path_;     ///< path to the logfile to write to

  AccessLog* access_log_;      ///< where we log accesses to
  FuseContext* fuse_context_;  ///< our fuse context
  fuse_chan* fuse_chan_;       ///< channel from fuse_mount
  fuse* fuse_;                 ///< fuse struct from fuse_new
  fuse_operations ops_;        ///< fuse operations
  bool use_mt_;                ///< use multi threaded loop

 public:
  MountPoint(const std::string& Run, const std::string& real_tree,
             const std::string& log_path);
  ~MountPoint();

  void Run(int argc, char** argv);

  /// calls fusermount -u
  /**
   *  note: it seems that it would be reasonable to call fuse_exit
   *  from the main thread (i.e. the caller, here) but there might
   *  be a kernel bug that would cause a memory leak. Instead, we are
   *  safe and make a system call to run "fusermount -u" which should
   *  do everything correctly.
   *
   *  see: http://sourceforge.net/mailarchive/message.php?msg_id=28221264
   */
  void Unmount();
};

}  // namespace logfs_fuse
