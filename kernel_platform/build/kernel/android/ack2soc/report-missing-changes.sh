#!/bin/bash
# Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

function report()
{
  local change='';

  local files_deleted=();
  local files_added=();
  local files_modified=();
  local files_other=();

  local files_added_trimmed=()
  local extra_trimming=()

  local -i deleted_count=0;
  local -i added_count=0;
  local -i modified_count=0;
  local -i other_count=0;

  local get_change=false;
  local is_merge=false;

  local -x GIT_DIR="${SOC_ROOT}/.git";
  local -x GIT_WORK_TREE="$SOC_ROOT";

  for change in "${CHANGES[@]}"; do
    get_change=false;

    if $REPORT_ALL; then
      # Limit deleted files to SOC files only!
      readarray -t files_deleted < <(
        git diff --name-only --no-renames --diff-filter=D "${change}^1" "$change" -- "${FILES[@]}";
      ) || return 1;

      readarray -t files_added < <(
        git diff --name-only --no-renames --diff-filter=A "${change}^1" "$change";
      ) || return 2;
    fi

    # Limit modified files to SOC files only!
    readarray -t files_modified < <(
      git diff --name-only --no-renames --diff-filter=M "${change}^1" "$change" -- "${FILES[@]}";
    ) || return 3;

    # PRECAUTION: Do the above cases cover all files touched by the change?
    #             Is it correct to limit those to SOC files only?
    readarray -t files_other < <(
      git diff --name-only --no-renames --diff-filter=dam "${change}^1" "$change";
    ) || return 4;

    is_merge="$(is_merge "$change")" || return 1;

    if $is_merge; then
      # NO merges are allowed! If any, we have to investigate them why they are
      # added instread of non-merge changes they are adding.
      printf "[SKIP] %s is merge commit\n";
    fi

    deleted_count="${#files_deleted[@]}";
    added_count="${#files_added[@]}";
    modified_count="${#files_modified[@]}";
    other_count="${#files_other[@]}";

    if [ $(($deleted_count + $added_count + $modified_count + $other_count)) -gt 0 ]; then
      get_change=true;
    fi

    git log --pretty=oneline "$change" -1;
    $get_change || {
      echo "^^^ Provides NO delta for SOC repo"
    }

    if $REPORT_ALL; then
      if [ $deleted_count -gt 0 ]; then
        echo "  For deleting: $deleted_count";
        printf "    %s\n" "${files_deleted[@]}";
      fi

      if [ $added_count -gt 0 ]; then
        # From the added by the change files, remove those that were already
        # added in the repo (on some reason) + remove blocklisted as well
        readarray -t files_added_trimmed < <(
          comm -23                                            \
            <(printf -- "%s\n" "${files_added[@]}" | sort -u) \
            <(printf -- "%s\n" "${FILES[@]}"       | sort -u) \
              | blocklist-filter;
        );

        echo "  For adding: ${#files_added_trimmed[@]} (change adds $added_count)";
        printf "    %s\n" "${files_added_trimmed[@]}" ;
      fi
    fi

    if [ $modified_count -gt 0 ]; then
      echo "  For modification: $modified_count";
      printf "    %s\n" "${files_modified[@]}";
    fi

    if [ $other_count -gt 0 ]; then
      echo "  Other files: $other_count";
      printf "    %s\n" "${files_other[@]}";
    fi
    echo
  done

  return 0;
}
