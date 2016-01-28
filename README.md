# A Logging Mirror Filesystem with FUSE

This is a simple fuse file system which provides a mirror of some source
directory in the tree rooted at the mountpoint, and logs all file accesses.

## Motivation

I had a flashable filesystem for an arm platform that I wanted to use as
a system-root for a cross-build, but the image was really big. So, I wanted
to strip the image down to just those parts I needed for the cross-build.
I wrote this filesystem to record a log of all the files in the rootfs that
were accessed during the cross-build, so I could audit them and delete
everything else.

## Usage:

~~~
logfs_fuse --real_tree <path> --mount_point <path> --log_path <path>
~~~

Where the arguments are:
  * *real_tree* : path to a directory whose contents to mirror
  * *mount_point* : path to the directory where we mount the mirror
  * *log_path* : path to a file where we write the access log

## Example:

We'll open three shells. In shell `a` we'll monitor the log file. In shell
`b` we'll run the fuse process. In shell `c` we'll do some manipulations on
files.

shell a:
~~~
~$ touch log.txt
~$ tail -f log.txt
create :/test.txt
getxattr :/test.txt
open :/test.txt
getxattr :/
getxattr :/
getxattr :/
opendir :/
getxattr :/test.txt
getxattr :/test.txt
getxattr :/test.txt
~~~

shell b:
~~~
~$ mkdir test_source
~$ mkdir test_mirror
~$ logfs_fuse --real_tree ./test_source --mount_point ./test_mirror --log_path ./log.txt
~~~

shell c:
~~~
~$ echo "Hello World" >> test_mirror/test.txt
~$ cat test_mirror/test.txt
Hello World
~$ ls -l test_mirror
total 4
-rw-rw-r-- 1 josh josh 12 Jan 28 00:25 test.txt
~$
~~~