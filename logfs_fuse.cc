#include <boost/filesystem.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "mount_point.h"

DEFINE_string(real_tree, "", "path to the directory tree to mirror");
DEFINE_string(mount_point, "", "path to the mount point of the mirror tree");
DEFINE_string(log_path, "/tmp/logfs_fuse.txt", "path of log-file to write to");

namespace fs = boost::filesystem;

const std::string kUsageMessage =
    "This is a simple fuse file system which provides a mirror of some source "
    "directory in the tree rooted at the mountpoint, and logs all file "
    "accesses.";

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetUsageMessage(kUsageMessage);
  google::ParseCommandLineFlags(&argc, &argv, true);

  fs::path real_tree_path(FLAGS_real_tree);
  fs::path mount_point_path(FLAGS_mount_point);
  fs::path log_path(FLAGS_log_path);

  LOG_IF(FATAL, !fs::exists(real_tree_path))
      << "Real tree to mirror '" << FLAGS_real_tree << "' doesn't exist";
  LOG_IF(FATAL, !fs::exists(mount_point_path))
      << "Mount point '" << FLAGS_mount_point << "' doesn't exist";
  LOG_IF(FATAL, !fs::exists(log_path.parent_path()))
      << "Parent directory of desired logfile '" << FLAGS_log_path
      << "' doesn't exist";

  logfs_fuse::MountPoint mount_point(FLAGS_mount_point, FLAGS_real_tree,
                                     FLAGS_log_path);
  mount_point.Run(argc, argv);
}
