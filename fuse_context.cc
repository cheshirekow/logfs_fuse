#include "fuse_context.h"

#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include "access_log.h"

namespace logfs_fuse {

static int ResultOrErrno(int result) {
  if (result < 0)
    return -errno;
  else
    return result;
}

std::string BoolStr(bool value) {
  return value ? "true" : "false";
}

FuseContext::FuseContext(const std::string& real_root, AccessLog* access_log)
    : access_log_(access_log), real_root_(real_root) {}

FuseContext::~FuseContext() {}

int FuseContext::mknod(const char* path, mode_t mode, dev_t dev) {
  access_log_->AddEntry("mknod", path);
  namespace fs = boost::filesystem;

  Path wrapped = (real_root_ / path);

  // we do not allow special files
  if (mode & (S_IFCHR | S_IFBLK))
    return -EINVAL;

  // create the local version of the file
  int result = ::mknod(wrapped.c_str(), mode, 0);
  if (result) {
    return -errno;
  }

  return 0;
}

int FuseContext::create(const char* path, mode_t mode,
                        struct fuse_file_info* fi) {
  access_log_->AddEntry("create", path);
  namespace fs = boost::filesystem;

  Path wrapped = (real_root_ / path);
  int fd = ::creat(wrapped.c_str(), mode);
  if (fd < 0) {
    return -errno;
  }

  fi->fh = fd;
  return 0;
}

int FuseContext::open(const char* path, struct fuse_file_info* fi) {
  access_log_->AddEntry("open", path);
  namespace fs = boost::filesystem;

  Path wrapped = real_root_ / path;
  int fd = ::open(wrapped.c_str(), fi->flags);
  if (fd < 0) {
    return -errno;
  }

  fi->fh = fd;
  return 0;
}

int FuseContext::read(const char* path, char* buf, size_t bufsize, off_t offset,
                      struct fuse_file_info* fi) {
  namespace fs = boost::filesystem;
  Path wrapped = (real_root_ / path);

  // if fi has a file handle then we simply read from the file handle
  if (fi->fh) {
    int result = ::pread(fi->fh, buf, bufsize, offset);
    if (result < 0) {
      return -errno;
    } else {
      return result;
    }
  } else {
    access_log_->AddEntry("read", path);

    // otherwise we open the file and perform the read
    // open the local version of the file
    int fd = ::open(wrapped.c_str(), O_RDONLY);
    if (fd < 0) {
      return -errno;
    }

    int result = ::pread(fd, buf, bufsize, offset);
    ::close(fd);

    if (result < 0) {
      return -errno;
    } else {
      return result;
    }
  }
}

int FuseContext::write(const char* path, const char* buf, size_t bufsize,
                       off_t offset, struct fuse_file_info* fi) {
  namespace fs = boost::filesystem;
  Path wrapped = (real_root_ / path);

  // if fi has a file handle then we simply read from the file handle
  if (fi->fh) {
    int result = ::pwrite(fi->fh, buf, bufsize, offset);
    if (result < 0) {
      return -errno;
    } else {
      return result;
    }
  } else {
    access_log_->AddEntry("write", path);

    // otherwise open the file
    int fd = ::open(wrapped.c_str(), O_WRONLY);
    if (fd < 0) {
      return -errno;
    }

    // perform the write
    for (ssize_t bytes_written = 0; bytes_written < bufsize;) {
      ssize_t result =
          ::pwrite(fd, buf + bytes_written, bufsize - bytes_written,
                   offset + bytes_written);
      if (result < 0) {
        return -errno;
      }
      bytes_written += result;
    }

    // close the file
    ::close(fd);
    return bufsize;
  }
}

int FuseContext::truncate(const char* path, off_t length) {
  access_log_->AddEntry("truncate", path);
  namespace fs = boost::filesystem;

  Path wrapped = (real_root_ / path).string();
  int result = ::truncate(wrapped.c_str(), length);
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::ftruncate(const char* path, off_t length,
                           struct fuse_file_info* fi) {
  namespace fs = boost::filesystem;

  Path wrapped = (real_root_ / path).string();
  if (fi->fh) {
    int result = ::ftruncate(fi->fh, length);
    if (result < 0) {
      return -errno;
    }

    return 0;
  } else {
    return -EBADF;
  }
}

int FuseContext::fsync(const char* path, int datasync,
                       struct fuse_file_info* fi) {
  Path wrapped = real_root_ / path;

  if (fi->fh) {
    if (datasync) {
      return ResultOrErrno(::fdatasync(fi->fh));
    } else {
      return ResultOrErrno(::fsync(fi->fh));
    }
  } else {
    return -EBADF;
  }
}

int FuseContext::flush(const char* path, struct fuse_file_info* fi) {
  Path wrapped = real_root_ / path;
  return 0;
}

int FuseContext::release(const char* path, struct fuse_file_info* fi) {
  if (fi->fh) {
    return ResultOrErrno(::close(fi->fh));
  } else {
    return -EBADF;
  }
}

int FuseContext::getattr(const char* path, struct stat* out) {
  namespace fs = boost::filesystem;
  Path wrapped = real_root_ / path;

  int result = ::lstat(wrapped.c_str(), out);
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::fgetattr(const char* path, struct stat* out,
                          struct fuse_file_info* fi) {
  namespace fs = boost::filesystem;
  Path wrapped = real_root_ / path;

  if (fi->fh) {
    int result = ::fstat(fi->fh, out);
    if (result < 0) {
      return -errno;
    }

    return 0;
  } else {
    return -EBADF;
  }
}

int FuseContext::unlink(const char* path) {
  access_log_->AddEntry("unlink", path);
  namespace fs = boost::filesystem;

  Path wrapped = real_root_ / path;

  // first we make sure that the parent directory exists
  Path parent = wrapped.parent_path();
  if (!fs::exists(parent))
    return -ENOENT;

  // unlink the directory holding the file contents, the meta file,
  // and the staged file
  fs::remove_all(wrapped);
  return 0;
}

int FuseContext::mkdir(const char* path, mode_t mode) {
  access_log_->AddEntry("mkdir", path);

  namespace fs = boost::filesystem;
  Path wrapped = real_root_ / path;

  // create the directory
  int result = ::mkdir(wrapped.c_str(), mode);
  if (result) {
    return -errno;
  }

  return 0;
}

int FuseContext::opendir(const char* path, struct fuse_file_info* fi) {
  access_log_->AddEntry("opendir", path);

  Path wrapped = real_root_ / path;
  DIR* result = ::opendir(wrapped.c_str());
  if (result == NULL) {
    return -errno;
  } else {
    fi->fh = reinterpret_cast<uint64_t>(result);
    return 0;
  }
}

int FuseContext::readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info* fi) {
  Path wrapped = real_root_ / path;

  if (!fi->fh) {
    return -EBADF;
  }

  DIR* dir = reinterpret_cast<DIR*>(fi->fh);
  seekdir(dir, offset);

  for (dirent* dir_entry = ::readdir(dir); dir_entry != NULL;
       dir_entry = ::readdir(dir)) {
    filler(buf, dir_entry->d_name, NULL, 0);

    // TODO(josh): learn how to properly use filler and offset
    //    if (filler(buf, dir_entry->d_name, NULL, ++offset)) {
    //      break;
    //    }
  }

  return 0;
}

int FuseContext::releasedir(const char* path, struct fuse_file_info* fi) {
  Path wrapped = real_root_ / path;

  if (fi->fh) {
    DIR* dir = reinterpret_cast<DIR*>(fi->fh);
    return ResultOrErrno(::closedir(dir));
  } else {
    return -EBADF;
  }
}

int FuseContext::fsyncdir(const char* path, int datasync,
                          struct fuse_file_info* fi) {
  access_log_->AddEntry("fsyncdir", path);
  Path wrapped = real_root_ / path;
  return 0;
}

int FuseContext::rmdir(const char* path) {
  access_log_->AddEntry("rmdir", path);
  namespace fs = boost::filesystem;

  Path wrapped = real_root_ / path;
  return ResultOrErrno(::rmdir(wrapped.c_str()));
}

int FuseContext::symlink(const char* oldpath, const char* newpath) {
  access_log_->AddEntry("symlink", newpath);

  Path oldwrap = real_root_ / oldpath;
  Path newwrap = real_root_ / newpath;

  int result = ::symlink(oldwrap.c_str(), newwrap.c_str());
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::readlink(const char* path, char* buf, size_t bufsize) {
  access_log_->AddEntry("readlink", path);

  Path wrapped = real_root_ / path;
  ssize_t result = ::readlink(wrapped.c_str(), buf, bufsize);
  if (result == ssize_t(-1)) {
    return -errno;
  }

  return 0;
}

int FuseContext::link(const char* oldpath, const char* newpath) {
  access_log_->AddEntry("link", std::string(oldpath) + " -> " + newpath);

  Path oldwrap = real_root_ / oldpath;
  Path newwrap = real_root_ / newpath;

  int result = ::link(oldwrap.c_str(), newwrap.c_str());
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::rename(const char* oldpath, const char* newpath) {
  access_log_->AddEntry("rename", std::string(oldpath) + " -> " + newpath);

  namespace fs = boost::filesystem;
  Path oldwrap = real_root_ / oldpath;
  Path newwrap = real_root_ / newpath;

  // if the move overwrites a file then copy data, increment version, and
  // unlink the old file
  int result = ::rename(oldwrap.c_str(), newwrap.c_str());
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::chmod(const char* path, mode_t mode) {
  access_log_->AddEntry("chmod", path);
  namespace fs = boost::filesystem;

  Path wrapped = real_root_ / path;
  int result = ::chmod(wrapped.c_str(), mode);
  if (result < 0)
    return -errno;

  return 0;
}

int FuseContext::chown(const char* path, uid_t owner, gid_t group) {
  access_log_->AddEntry("chmod", path);
  namespace fs = boost::filesystem;

  Path wrapped = (real_root_ / path);
  int result = ::chown(wrapped.c_str(), owner, group);
  if (result < 0)
    return -errno;

  return 0;
}

int FuseContext::access(const char* path, int mode) {
  access_log_->AddEntry("access", path);

  Path wrapped = real_root_ / path;
  int result = ::access(wrapped.c_str(), mode);
  if (result < 0)
    return -errno;

  return 0;
}

int FuseContext::lock(const char* path, struct fuse_file_info* fi, int cmd,
                      struct flock* fl) {
  access_log_->AddEntry("lock", path);

  Path wrapped = real_root_ / path;
  if (fi->fh) {
    int result = fcntl(fi->fh, cmd, fl);
    if (result < 0) {
      return -errno;
    }
    return 0;
  } else {
    return -EBADF;
  }
}

int FuseContext::utimens(const char* path, const struct timespec tv[2]) {
  access_log_->AddEntry("utimens", path);

  Path wrapped = real_root_ / path;
  timeval times[2];
  for (int i = 0; i < 2; i++) {
    times[i].tv_sec = tv[i].tv_sec;
    times[i].tv_usec = tv[i].tv_nsec / 1000;
  }

  int result = ::utimes(wrapped.c_str(), times);
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::statfs(const char* path, struct statvfs* buf) {
  access_log_->AddEntry("statfs", path);

  Path wrapped = real_root_ / path;
  int result = ::statvfs(wrapped.c_str(), buf);
  if (result < 0) {
    return -errno;
  }

  return 0;
}

int FuseContext::setxattr(const char* path, const char* key, const char* value,
                          size_t bufsize, int flags) {
  access_log_->AddEntry("setxattr", path);

  Path wrapped = real_root_ / path;
  int result = ::setxattr(wrapped.c_str(), key, value, bufsize, flags);
  if (result < 0) {
    return -errno;
  }
  return 0;
}

int FuseContext::getxattr(const char* path, const char* key, char* value,
                          size_t bufsize) {
  access_log_->AddEntry("getxattr", path);

  Path wrapped = real_root_ / path;
  int result = ::getxattr(wrapped.c_str(), key, value, bufsize);
  if (result < 0) {
    return -errno;
  }
  return 0;
}

int FuseContext::listxattr(const char* path, char* buf, size_t bufsize) {
  access_log_->AddEntry("listxattr", path);

  Path wrapped = real_root_ / path;
  int result = ::listxattr(wrapped.c_str(), buf, bufsize);
  if (result < 0) {
    return -errno;
  }
  return 0;
}

int FuseContext::removexattr(const char* path, const char* key) {
  access_log_->AddEntry("removexattr", path);

  Path wrapped = real_root_ / path;
  int result = ::removexattr(wrapped.c_str(), key);
  if (result < 0) {
    return -errno;
  }
  return 0;
}

int FuseContext::bmap(const char*, size_t blocksize, uint64_t* idx) {
  return -EINVAL;
}

int FuseContext::ioctl(const char* path, int cmd, void* arg,
                       struct fuse_file_info* fi, unsigned int flags,
                       void* data) {
  return -EINVAL;
}

int FuseContext::poll(const char*, struct fuse_file_info*,
                      struct fuse_pollhandle* ph, unsigned* reventsp) {
  return -EINVAL;
}

}  // namespace logfs_fuse
