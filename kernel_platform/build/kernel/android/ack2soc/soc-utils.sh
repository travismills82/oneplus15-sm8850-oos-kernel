#!/bin/bash
# Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

function get-soc-tip()
{
  local tip_regex="refs/heads/${SOC_BRANCH#refs/heads/}\$";

  git ls-remote -h "$SOC_URL" \
    | grep -P  "$tip_regex"   \
    | grep -Po '^[^\s]+';
}

function soc-files()
{
  files "$SOC_ROOT" "$SOC_SHA" "${DRIVERS[@]}" "${PATHS[@]}";
}

function ack-files()
{
  # TODO: This should be corrected to ACK_SHA?
  files "$ACK_ROOT" "$ACK_SHA_KP" "${DRIVERS[@]}" "${PATHS[@]}";
}

#
# Load common files between SOC and ACK (state @ACK_SHA_KP) limited by:
# 1. Cmdline:
#      --driver options
#      Any paths provided as free arguments
# 2. BLOCKLIST array
#
function soc-monitored-files()
{
  $DEBUG && set -x;

  comm -12                 \
    <(soc-files | sort -u) \
    <(ack-files | sort -u) \
      | blocklist-filter;
}

#
# This function dumps list of all files, which are part of SOC repo. It embeds
# the rules to prepare the list.
#
function repo_soc_files()
{
  git -C "$SOC_ROOT" show HEAD:"$SOC_FILE_LIST" \
    | grep -P '\.(c|h)'                         \
    | grep -Po '^\s+"\s*\K[^"]+';
}

#
# When doing git merge|cherry-pick, we need to remove those files with
# conflicts, which are NOT SOC related i.e. need to discard the garbage
# added on importing upstream changes into SOC repo.
#
# This function dumps the above described list of files.
#
function list-no-soc-files()
{
  # Remove:
  # 1) 'DD|DU'
  #    Those files with conflicts which were deleted in SOC repo
  #
  # 2) 'A '
  #    Remove newly added by the merge
  #
  # 3) 'UA'
  #    Exists on the merge side but the change which adds the file
  #    is discarded on some reason and now delta is added into that
  #    file

  git -C "$SOC_ROOT" status --porcelain \
    | sort                              \
    | grep -Po '(?<=^(DD|DU|A |UA) ).+$';
}

function discard-no-soc-files()
{
  local no_soc_files=();

  local -x GIT_DIR="${SOC_ROOT}/.git";
  local -x GIT_WORK_TREE="$SOC_ROOT";

  readarray -t no_soc_files < <(list-no-soc-files);

  if [[ ${#no_soc_files[@]} -gt 0 ]]; then
    $DEBUG && set -x;

    git rm -rf -- "${no_soc_files[@]}";

    $DEBUG && set +x;
  fi

  # TODO: Can we add here a wiki or some info, which describes why bindings
  #       directory is discarded i.e. converted to symlink?
  if [[ ! -L $DEVICETREE_BINDINGS_PATH ]]; then
    $DEBUG && set -x;

    git rm -rf        -- "${DEVICETREE_BINDINGS_PATH}/";
    git checkout HEAD -- "${DEVICETREE_BINDINGS_PATH}";

    $DEBUG && set +x;
  fi
}

function list-drivers()
{
  local root_dir="$1"; shift 1;

  (
    cd "$root_dir" || exit 1;

    $DEBUG && set -x;

    find . -type f -iname "$DRIVER_MARKER" \
      | trim-paths;
  )
}
