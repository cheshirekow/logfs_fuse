cmake_minimum_required(VERSION 2.8)
project (logfs_fuse)

find_package(PkgConfig)
pkg_check_modules(fuse REQUIRED fuse)

add_executable(logfs_fuse logfs_fuse.cc)
target_include_directories(logfs_fuse PRIVATE ${fuse_INCLUDE_DIRS})
target_link_libraries(logfs_fuse ${fuse_LDFLAGS})
