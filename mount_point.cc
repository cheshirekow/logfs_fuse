#include <gflags/gflags.h>
#include <glog/logging.h>

#include "access_log.h"
#include "fuse_context.h"
#include "fuse_operations.h"
#include "mount_point.h"

namespace logfs_fuse {

MountPoint::MountPoint(const std::string& mount, const std::string& real_tree,
                       const std::string& log_path)
    : mount_point_(mount),
      real_tree_(real_tree),
      log_path_(log_path),
      fuse_chan_(0),
      fuse_(0),
      use_mt_(false) {}

MountPoint::~MountPoint() {
  // fuse_context_ will be destoyed by the destroy fuse op that we
  // implemented
  // delete fuse_context_;
  delete access_log_;
}

void MountPoint::Run(int argc, char** argv) {
  // fuse arguments
  fuse_args args = {argc, argv, 0};

  // create the mount point
  fuse_chan_ = fuse_mount(mount_point_.c_str(), &args);
  if (!fuse_chan_)
    LOG(FATAL) << "Failed to fuse_mount " << mount_point_;

  // initialize fuse_ops
  SetFuseOps(&ops_);

  // create initializer object which is passed to fuse_ops::init
  access_log_ = new AccessLog(log_path_);
  fuse_context_ = new FuseContext(real_tree_, access_log_);

  // initialize fuse
  fuse_ = fuse_new(fuse_chan_, &args, &ops_, sizeof(ops_), fuse_context_);
  if (!fuse_) {
    fuse_unmount(mount_point_.c_str(), fuse_chan_);
    fuse_chan_ = 0;
    LOG(FATAL) << "Failed to fuse_new";
  }

  // start the main fuse loop
  LOG(INFO) << "MountPoint::main: " << static_cast<void*>(this)
            << "entering fuse loop\n";

  if (use_mt_)
    fuse_loop_mt(fuse_);
  else
    fuse_loop(fuse_);

  LOG(INFO) << "MountPoint::main: " << static_cast<void*>(this)
            << "exiting fuse loop\n";
  fuse_unmount(mount_point_.c_str(), fuse_chan_);
  fuse_destroy(fuse_);
}

void MountPoint::Unmount() {
  std::string cmd = "fusermount -u " + mount_point_;
  std::cout << "Mointpoint::unmount: " << cmd;

  int result = system(cmd.c_str());
  if (result < 0) {
    LOG(FATAL) << "Failed to unmount " << mount_point_;
  }
}

}  // namespace logfs_fuse
