#!/usr/bin/python
"""Clone a subset of a filesystem using a list of file names specifying what to
   copy."""

import argparse
import errno
import os
import shutil
import sys
import time


def fmt_out(fmt_str, *args, **kwargs):
  """Write the string to stdout and flush."""

  sys.stdout.write(fmt_str.format(*args, **kwargs))
  sys.stdout.flush()


def force_symlink(target, link_path):
  """Force the creation of a symlink, removing the link path if it already
     exists."""
  try:
    os.symlink(target, link_path)
  except OSError, exception:
    if exception.errno == errno.EEXIST:
      os.remove(link_path)
      os.symlink(target, link_path)

def copy_path(src_path, dst_path):
  """Copy a symlink or a file."""

  dst_parent = os.path.dirname(dst_path)
  if not os.path.exists(dst_parent):
    os.makedirs(dst_parent)

  if os.path.islink(src_path):
    copy_link(src_path, dst_path)
  elif os.path.isfile(src_path):
    shutil.copyfile(src_path, dst_path)

def copy_link(src_link, dst_link):
  """Copy the target of a symlink from src_link to dst_link. Do this
     recursively if the target is also a symlink."""
  src_dir = os.path.dirname(src_link)
  dst_dir = os.path.dirname(dst_link)

  target = os.readlink(src_link)
  force_symlink(target, dst_link)

  # Don't want to edit any files outside dst_dir
  if target.startswith('/'):
    return

  src_target = os.path.join(src_dir, target)
  dst_target = os.path.join(dst_dir, target)
  copy_path(src_target, dst_target)

def copy_files(src_dir, dst_dir, infile):
  """See module docsting."""

  infile.seek(0, 2)
  infile_size = infile.tell()
  infile.seek(0, 0)

  last_print_time = 0
  num_lines = 0
  for line in iter(infile.readline, b''):
    filename = line.strip()[1:]
    src_path = os.path.join(src_dir, filename)
    dst_path = os.path.join(dst_dir, filename)

    copy_path(src_path, dst_path)
    num_lines += 1

    now = time.time()
    if now - last_print_time > 0.1:
      fraction = 100 * infile.tell() / float(infile_size)
      last_print_time = now
      fmt_out('\rCopied {} files: {:6.2f}%', num_lines, fraction)
  fmt_out('\rCopied {} files: 100.00%\n', num_lines)

  return 0

DESCRIPTION_STRING = """
Clone a subset of a filesystem using a list of file names specifying what to
copy.

Supply "-" (hyphen/minus) as the input file to read from stdin.
"""

def main():
  """Just parses arguments and then calls copy_files."""

  parser = argparse.ArgumentParser(description=DESCRIPTION_STRING)
  parser.add_argument('-i', '--input-file', default='-',
                      help='logfs_fuse generated access log')
  parser.add_argument('-s', '--src', help='source directory')
  parser.add_argument('-d', '--dest', help='destionation directory')
  args = parser.parse_args()
  if args.input_file == '-':
    infile = sys.stdin
  else:
    infile = open(args.input_file, 'r')

  exit_code = copy_files(args.src, args.dest, infile)
  sys.exit(exit_code)

if __name__ == '__main__':
  main()
