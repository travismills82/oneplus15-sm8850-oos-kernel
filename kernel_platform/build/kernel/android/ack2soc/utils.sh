#!/bin/bash
# Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

function cleanup()
{
  local verbose='';

  $DEBUG && verbose='v';
  rm -Rf${verbose} "${DELETE_QUEUE[@]}" >&2;
}

function tt-ff()
{
  if [[ -z $1 ]]; then
    echo false;
    return 1;
  fi

  (
    shopt -s  nocasematch;

    case "$1" in
      false|ff|f|0|no|n)
        echo false;
        return 1;
      ;;
      *)
        echo true;
        return 0;
      ;;
    esac
  )
}

function true-false() { tt-ff "$@"; }

function yes-no()
{
  tt-ff "$@" \
    | sed -e 's/true/yes/; s/false/no/';

  return "${PIPESTATUS[0]}";
}

function debug_on()
{
  exec 2>> "$DEBUG_LOG"; set -x;
}

function debug_off()
{
  set +x; exec 2>&1;
}

function _mktemp()
{
  local prefix="$1"; shift 1;

  mktemp --dry-run --tmpdir "${prefix}_XXXXXXXXX" \
    2>/dev/null                                   \
      && return 0;

  # If above command fails on some reason use this
  echo "${PWD}/${prefix}_$PPID.txt";
}

function trim-paths()
{
  local regexes=(
    '\./'       # Current folder
    '/[^/]+$'   # Last component from the path
  )
  local IFS='|';

  sed --regexp-extended -e "s#${regexes[*]}##g";
}

function blocklist-regexes()
{
  local path='';

  for path in "${BLOCKLIST[@]}"; do
    BLOCKLIST_REGEXES+=('^\Q'"${path}"'\E($|/)')
  done
}

function blocklist-filter()
{
  local IFS='|';

  $DEBUG && set -x;

  grep -Pv -- "${BLOCKLIST_REGEXES[*]}";

  $DEBUG && set +x;
}

#
# This function lists against provided SHA all _existing_ files and expands
# patterns to all _existing_ files from what is loaded in arrays $PATHS and
# $DRIVERS. Said other way, in a provied SHA, non-existing files are ommitted
# and patterns are replaced by the list of existing files in that SHA.
#
# The goal of that function is to filter-out non-existing files and expands
# path patterns by checking against provided SHA.
#
function files()
{
  local dir="$1"; shift 1;
  local sha="$1"; shift 1;

  $DEBUG && debug_on;

  git -C "$dir"     \
    ls-tree         \
    -r              \
    --name-only     \
    "$sha"          \
    --              \
    "$@";

  $DEBUG && {
    debug_off;
    $INTERACTIVE && read -p "To continue press [enter] key ...";
  }
}

# This is a shell wrapper to `git log ...` command
#
# Parameters:
#   $1 - Repository path
#   $2 - Left commit, optional, if missed HEAD is assumed
#   $3 - Right commit
#   $4 - Options to git log command
#
function git-log()
{
  local repo="$1"; shift 1;
  local left_commit='HEAD';
  local right_commit='';
  local git_log_options=();

  local -i status=0;
  local sha_regex='^[0-9a-f]{7,40}\b';

  if echo "$1" | grep -Pqi "$sha_regex"; then
    right_commit="$1"; shift 1;
    if echo "$1" | grep -Pqi "$sha_regex"; then
      left_commit="$right_commit";
      right_commit="$1"; shift 1;
    fi
  fi

  [ -z "$right_commit" ] && return 1;

  git_log_options=("$@");

  $DEBUG && debug_on;

  git -C "$repo" log                  \
    --oneline                         \
    --pretty=%H                       \
    "${git_log_options[@]}"           \
    "${left_commit}..${right_commit}" \
    --                                \
    "${FILES[@]}"         || status=$?;

  $DEBUG && debug_off;

  return $status;
}

function list-first-parent-changes()
{
  git-log "$@" --first-parent;
}

# This function prints all non-merge changes in reverse order between two
# commits, by preserving the order of their appearance in --first-parent level
# from the range. Order is as follows:
#
# 1) The commit date, if change is non-merge and on --first-parent level
# 2) If change is a MERGE commit it is expanded recursively to list of all non-
#    merge changes in reverse order by preserving the order of their appearance
#    in the range between merge base (of MERGE^1 and MERGE^2) and MERGE^2
#
# TODO: Seems `git log --topo-order' is equivalent, but need more testing to be
#       confirmed for sure.
#
function list-changes()
{
  _list-changes "$@" --reverse 1;
}

function _list-changes()
{
  local args=("$@");

  local repo="${args[0]}";   unset args[0];
  local level="${args[-1]}"; unset args[-1];

  local changes=()      change='';
  local parents=()      parent='';
  local merge_bases=()  merge_base='';

  local is_merge='false';

  # Make sure, if not set on the cmdline, all git commands to be against $repo,
  local -x GIT_DIR="${repo}/.git";
  local -x GIT_WORK_TREE="$repo";

  exec 2>> "$DEBUG_LOG";

  readarray -t changes < <(
    list-first-parent-changes "$repo" "${args[@]}";
  ) || return 1;

  echo "Processing LEVEL $level: ${#changes[@]} changes" >&2;
  for change in "${changes[@]}"; do

    [[ -v CHANGES_ALL[$change] ]] || {
      echo "$change (from ${args[@]}, level $level) is not in the range" >&2;
      continue;
    } >&2

    [[ ${CHANGES_ALL[$change]} = 1 ]] && {
      echo "CYCLE at $change" >&2;
      continue;
    }

    CHANGES_ALL[$change]=1;

    readarray -t parents < <(
      commit_parents "$change";
    ) || {
      echo "ERROR: Can't inspect SHA $change" >&2;
      return 1;
    }

    if [[ ${#parents[@]} -ge 2 ]]; then
      for parent in "${parents[@]:1}"; do
        readarray -t merge_bases < <(
          git merge-base --all "${parents[0]}" "$parent";
        ) || return 2;

        for merge_base in "${merge_bases[@]}"; do
          _list-changes "$repo" "$merge_base" "$parent" --reverse "$(($level+1))";
        done
      done
    else
      CHANGES+=("$change");
      $PROGRESS &&                                                   \
        printf                                                       \
          --                                                         \
          "\rTOTAL: %7s | NON_MERGE: %7s | LEVEL: %3s | SHA: %7.7s"  \
          "$(printf -- "%s\n" "${CHANGES_ALL[@]}" | grep 1 | wc -l)" \
          "${#CHANGES[*]}"                                           \
          "$level"                                                   \
          "$change" |& tee /dev/tty;
    fi
  done

  return 0;
}

function commit_parents()
{
  local commit_sha="$1";
  (
    set -o pipefail;
    git log --pretty='%P' "$commit_sha" -1 | tr ' ' '\n';
  )
}

function is_merge()
{
  local sha="$1"; shift 1;

  local -i parents=1;

  parents="$(
    commit_parents "$sha" | wc -l;
  )" || return 1;

  if [[ $parents -eq 1 ]]; then
    echo false; # regular commit
    return 0;
  elif [[ $parents -ge 2 ]]; then
    echo true;  # merge commit
    return 0;
  fi

  return 2;
}

function git-add-alternate()
{
  local base_repo="${1%/.git}/.git";   shift 1;
  local target_repo="${1%/.git}/.git"; shift 1;

  local target_objects="$target_repo/objects";
  local alternates_file="$base_repo/objects/info/alternates";

  [ -d "$base_repo" -a -d "$target_objects" ] || return 1;

  test -f "$alternates_file" || {
    mkdir -pv "$(dirname "$alternates_file")" || return 2;
    touch "$alternates_file"                  || return 3;
  }

  # Add the path if missing
  cat "$alternates_file" \
    | grep --quiet -F -- "$target_objects" || {
      echo "$target_objects" \
        >> "$alternates_file" || return 4;
  }
}

