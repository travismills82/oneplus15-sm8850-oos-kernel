#!/usr/bin/env python3
#
# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""A script to copy outputs from Bazel rules to a user specified dist directory.

This script is only meant to be executed with `bazel run`. `bazel build <this
script>` doesn't actually copy the files, you'd have to `bazel run` a
copy_to_dist_dir target.

This script copies files from Bazel's output tree into a directory specified by
the user. It does not check if the dist dir already contains the file, and will
simply overwrite it.

One approach is to wipe the dist dir every time this script runs, but that may
be overly destructive and best left to an explicit rm -rf call outside of this
script.

Another approach is to error out if the file being copied already exist in the
dist dir, or perform some kind of content hash checking.
"""

import argparse
import collections
import fnmatch
import glob
import logging
import os
import pathlib
import shutil
import sys
import tarfile
import textwrap


_CMDLINE_FLAGS_SENTINEL = "CMDLINE_FLAGS_SENTINEL"

# Arguments that should not be specified in the command line, but only
# in BUILD files.
_DEPRECATED_CMDLINE_OPTIONS = {
    "--dist_dir": "Use --destdir instead.",
    "--log": "Use -q instead.",
    "--archive_prefix": "",
    "--flat": "Specify it in the BUILD file (e.g. copy_to_dist_dir(flat=True))",
    "--strip_components": "Specify it in the BUILD file "
                          "(e.g. copy_to_dist_dir(strip_components=1))",
    "--prefix": "Specify it in the BUILD file "
                "(e.g. copy_to_dist_dir(prefix='prefix'))",
    "--wipe_dist_dir": "Specify it in the BUILD file "
                       "(e.g. copy_to_dist_dir(wipe_dist_dir=True))",
    "--allow_duplicate_filenames":
        "Specify it in the BUILD file "
        "(e.g. copy_to_dist_dir(allow_duplicate_filenames=True))",
    "--mode_override":
        "Specify it in the BUILD file "
        "(e.g. copy_to_dist_dir(mode_overrides=[('*.sh', '755')]))",
}


def copy_with_modes(src, dst, mode_overrides):
    mode_override = None
    for (pattern, mode) in mode_overrides:
        if fnmatch.fnmatch(src, pattern):
            mode_override = mode
            break

    # Remove destination file that may be write-protected
    pathlib.Path(dst).unlink(missing_ok=True)

    # Copy the file with copy2 to preserve whatever permissions are set on src
    shutil.copy2(os.path.abspath(src), dst, follow_symlinks=True)

    if mode_override:
        os.chmod(dst, mode_override)


def ensure_unique_filenames(files):
    basename_to_srcs_map = collections.defaultdict(list)
    for f in files:
        basename_to_srcs_map[os.path.basename(f)].append(f)

    duplicates_exist = False
    for (basename, srcs) in basename_to_srcs_map.items():
        if len(srcs) > 1:
            duplicates_exist = True
            logging.error('Destination filename "%s" has multiple possible sources: %s',
                         basename, srcs)

    if duplicates_exist:
        sys.exit(1)


def files_to_dist(pattern):
    # Assume that dist.bzl is in the same package as dist.py
    runfiles_directory = os.path.dirname(__file__)
    dist_manifests = glob.glob(
        os.path.join(runfiles_directory, pattern))
    if not dist_manifests:
        logging.warning("Could not find a file with pattern %s"
                        " in the runfiles directory: %s", pattern, runfiles_directory)
    files_to_dist = []
    for dist_manifest in dist_manifests:
        with open(dist_manifest, "r") as f:
            files_to_dist += [line.strip() for line in f]
    return files_to_dist


def copy_files_to_dist_dir(files, archives, mode_overrides, dist_dir, flat, prefix,
    strip_components, archive_prefix, wipe_dist_dir, allow_duplicate_filenames, **ignored):

    if flat and not allow_duplicate_filenames:
        ensure_unique_filenames(files)

    if wipe_dist_dir and os.path.exists(dist_dir):
        shutil.rmtree(dist_dir)

    logging.info("Copying to %s", dist_dir)

    for src in files:
        if flat:
            src_relpath = os.path.basename(src)
        elif strip_components > 0:
            src_relpath = src.split('/', strip_components)[-1]
        else:
            src_relpath = src

        src_relpath = os.path.join(prefix, src_relpath)

        dst = os.path.join(dist_dir, src_relpath)
        if os.path.isfile(src):
            dst_dirname = os.path.dirname(dst)
            logging.debug("Copying file: %s" % dst)
            if not os.path.exists(dst_dirname):
                os.makedirs(dst_dirname)

            copy_with_modes(src, dst, mode_overrides)
        elif os.path.isdir(src):
            logging.debug("Copying dir: %s" % dst)
            if os.path.exists(dst):
                # make the directory temporary writable, then
                # shutil.copytree will restore correct permissions.
                os.chmod(dst, 750)
            shutil.copytree(
                os.path.abspath(src),
                dst,
                copy_function=lambda s, d: copy_with_modes(s, d, mode_overrides),
                dirs_exist_ok=True,
            )

    for archive in archives:
        try:
            with tarfile.open(archive) as tf:
                dst_dirname = os.path.join(dist_dir, archive_prefix)
                logging.debug("Extracting archive: %s -> %s", archive, dst_dirname)
                tf.extractall(dst_dirname)
        except tarfile.TarError:
            # toybox does not support creating empty tar files, hence the build
            # system may use empty files as empty archives.
            if os.path.getsize(archive) == 0:
                logging.warning("Skipping empty tar file: %s", archive)
                continue
             # re-raise if we do not know anything about this error
            logging.exception("Unknown TarError.")
            raise


def config_logging(log_level_str):
    level = getattr(logging, log_level_str.upper(), None)
    if not isinstance(level, int):
        sys.stderr.write("ERROR: Invalid --log {}\n".format(log_level_str))
        sys.exit(1)
    logging.basicConfig(level=level, format="[dist] %(levelname)s: %(message)s")


class CheckDeprecationAction(argparse.Action):
    """Checks if a deprecated option is used, then do nothing."""
    def __call__(self, parser, namespace, values, option_string=None):
        if option_string in _DEPRECATED_CMDLINE_OPTIONS:
            logging.warning("%s is deprecated! %s", option_string,
                            _DEPRECATED_CMDLINE_OPTIONS[option_string])


class StoreAndCheckDeprecationAction(CheckDeprecationAction):
    """Sotres the value, and checks if a deprecated option is used."""
    def __call__(self, parser, namespace, values, option_string=None):
        super().__call__(parser, namespace, values, option_string)
        setattr(namespace, self.dest, values)


class StoreTrueAndCheckDeprecationAction(CheckDeprecationAction):
    """Sotres true, and checks if a deprecated option is used."""
    def __call__(self, parser, namespace, values, option_string=None):
        super().__call__(parser, namespace, values, option_string)
        setattr(namespace, self.dest, True)


class AppendAndCheckDeprecationAction(CheckDeprecationAction):
    """Appends the value, and checks if a deprecated option is used."""
    def __call__(self, parser, namespace, values, option_string=None):
        super().__call__(parser, namespace, values, option_string)
        if not values:
            return
        metavar_len = len(self.metavar)if self.metavar else 1
        value_groups = [values[i:i + metavar_len]
                        for i in range(0, len(values), metavar_len)]
        setattr(namespace, self.dest,
                getattr(namespace, self.dest, []) + value_groups)


def _get_parser(cmdline=False) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Dist Bazel output files into a custom directory.",
        formatter_class=argparse.RawTextHelpFormatter)
    deprecated = parser.add_argument_group(
        "Deprecated command line options",
        description=textwrap.dedent("""\
            List of command line options that are deprecated.
            Most of them should be specified in the BUILD file instead.
        """))
    parser.add_argument(
        "--destdir", "--dist_dir", required=not cmdline, dest="dist_dir",
        help=textwrap.dedent("""\
            path to the dist dir.
            If relative, it is interpreted as relative to Bazel workspace root
            set by the BUILD_WORKSPACE_DIRECTORY environment variable, or
            PWD if BUILD_WORKSPACE_DIRECTORY is not set.

            Note: --dist_dir is deprecated; use --destdir instead."""),
        action=StoreAndCheckDeprecationAction if cmdline else "store")
    deprecated.add_argument(
        "--flat",
        action=StoreTrueAndCheckDeprecationAction if cmdline else "store_true",
        help="ignore subdirectories in the manifest")
    deprecated.add_argument(
        "--strip_components", type=int, default=0,
        help="number of leading components to strip from paths before applying --prefix",
        action=StoreAndCheckDeprecationAction if cmdline else "store")
    deprecated.add_argument(
        "--prefix", default="",
        help="path prefix to apply within dist_dir for copied files",
        action=StoreAndCheckDeprecationAction if cmdline else "store")
    deprecated.add_argument(
        "--archive_prefix", default="",
        help="Path prefix to apply within dist_dir for extracted archives. " +
             "Supported archives: tar.",
        action=StoreAndCheckDeprecationAction if cmdline else "store")
    deprecated.add_argument("--log", help="Log level (debug, info, warning, error)",
        default="debug",
        action=StoreAndCheckDeprecationAction if cmdline else "store")
    parser.add_argument("-q", "--quiet", action="store_const", default=False,
                        help="Same as --log=error", const="error", dest="log")
    deprecated.add_argument(
        "--wipe_dist_dir",
        action=StoreTrueAndCheckDeprecationAction if cmdline else "store_true",
        help="remove existing dist_dir prior to running",
    )
    deprecated.add_argument(
        "--allow_duplicate_filenames",
        action=StoreTrueAndCheckDeprecationAction if cmdline else "store_true",
        help="allow multiple files with the same name to be copied to dist_dir (overwriting)"
    )
    deprecated.add_argument(
        "--mode_override",
        metavar=("PATTERN", "MODE"),
        action=AppendAndCheckDeprecationAction if cmdline else "append",
        nargs=2,
        default=[],
        help='glob pattern and mode to set on files matching pattern (e.g. --mode_override "*.sh" "755")'
    )
    return parser

def main():
    args = sys.argv[1:]
    args.remove(_CMDLINE_FLAGS_SENTINEL)
    args = _get_parser().parse_args(args)

    config_logging(args.log)

    # Warn about arguments that should not be set in command line.
    _get_parser(cmdline=True).parse_args(
        sys.argv[sys.argv.index(_CMDLINE_FLAGS_SENTINEL) + 1:])

    mode_overrides = []
    for (pattern, mode) in args.mode_override:
        try:
            mode_overrides.append((pattern, int(mode, 8)))
        except ValueError:
            logging.error("invalid octal permissions: %s", mode)
            sys.exit(1)

    if not os.path.isabs(args.dist_dir):
        # BUILD_WORKSPACE_DIRECTORY is the root of the Bazel workspace containing
        # this binary target.
        # https://docs.bazel.build/versions/main/user-manual.html#run
        args.dist_dir = os.path.join(
            os.environ.get("BUILD_WORKSPACE_DIRECTORY"), args.dist_dir)

    files = files_to_dist("*_dist_manifest.txt")
    archives = files_to_dist("*_dist_archives_manifest.txt")
    copy_files_to_dist_dir(files, archives, mode_overrides, **vars(args))


if __name__ == "__main__":
    main()
