#include <list>

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

std::list<std::string> StripFuseFlags(int* argc, char*** argv) {
  std::list<std::string> fuse_flags;
  int write_idx = 0;
  for (int read_idx = 0; read_idx < *argc; ++read_idx) {
    std::string arg = (*argv)[read_idx];
    if (arg == "-o") {
      fuse_flags.push_back(arg);
      if (read_idx < *argc) {
        ++read_idx;
        fuse_flags.push_back((*argv)[read_idx]);
      }
    } else {
      (*argv)[write_idx++] = (*argv)[read_idx];
    }
  }

  *argc = write_idx;
  return fuse_flags;
}

std::vector<char*> MakeArgv(std::list<std::string>* argv_list) {
  std::vector<char*> argv;
  argv.reserve(argv_list->size() + 1);
  for (std::string& arg : (*argv_list)) {
    argv.push_back(&arg[0]);
  }
  argv.push_back(NULL);

  return argv;
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  std::list<std::string> fuse_flags = StripFuseFlags(&argc, &argv);
  std::vector<char*> fuse_args = MakeArgv(&fuse_flags);

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

  int fuse_argc = fuse_args.size() - 1;
  char** fuse_argv = fuse_args.data();
  mount_point.Run(fuse_argc, fuse_argv);
}
