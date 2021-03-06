cmake_minimum_required(VERSION 2.8)
project (logfs_fuse)

find_package(PkgConfig)
pkg_check_modules(fuse REQUIRED fuse)
pkg_check_modules(glog REQUIRED libglog)
pkg_check_modules(gflags REQUIRED libgflags_nothreads)

find_package(Boost REQUIRED COMPONENTS filesystem system)
find_package(PythonInterp)

set(clang_versions 3.6 3.7 3.8 3.9)
set(clang_format_names)
foreach(clang_version ${clang_versions})
  list(APPEND clang_format_names clang-format-${clang_version})
endforeach()

find_program(clang_format NAMES ${clang_format_names})

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
add_definitions(-D_FILE_OFFSET_BITS=64)

file(GLOB logfs_fuse_sources *.h *.cc)
add_executable(logfs_fuse ${logfs_fuse_sources})

target_include_directories(logfs_fuse PRIVATE
  ${Boost_INCLUDE_DIR}
  ${fuse_INCLUDE_DIRS}
  ${gflags_INCLUDE_DIRS}
  ${glog_INCLUDE_DIRS})

target_link_libraries(logfs_fuse
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${fuse_LDFLAGS}
  ${gflags_LDFLAGS}
  ${glog_LDFLAGS})

if(PYTHONINTERP_FOUND)
  set(cpplint ${CMAKE_CURRENT_SOURCE_DIR}/cpplint.py)
  add_custom_target(lint
    COMMAND ${PYTHON_EXECUTABLE} ${cpplint} ${logfs_fuse_sources})
endif()

if(clang_format)
  add_custom_target(format
    COMMAND ${clang_format} -i -style=File ${logfs_fuse_sources})
endif()
