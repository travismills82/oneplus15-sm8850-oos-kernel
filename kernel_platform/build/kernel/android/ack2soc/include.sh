#!/bin/bash
# Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

# This script aims in sourcing multiple files, by printing well formatted error
# messages and has the potential to be extended. The key point is to be
# source-d and files which we would like to source to be passed as arguments.
#
# Here is an example how to embed it into calling script:
#
# INCLUDE_PATH="$(
#   realpath -m \
#    "$(dirname "$0")/$(basename "${0%.sh}")"
# )" || exit 1;
#
# INCLUDE_CMD='include.sh';
#
# INCLUDES=(
#   "${INCLUDE_PATH}/env.sh"
#   "${INCLUDE_PATH}/cmdline.sh"
#   ...
# );
#
# HINT: If not worry regarding the order and unexpected files use below which
#       will give the flexibility to extend the functionality without worry
#       regarding typos in file names and editing the file, which imports them,
#       each time on adding new one.
#
# readarray -t INCLUDES < <(
#  find "${INCLUDE_PATH}" \
#      -name '*.sh'       \
#    ! -name "$INCLUDE_CMD"
# );
#
# source "${INCLUDE_PATH}/${INCLUDE_CMD}" "${INCLUDES[@]}" || exit 1;

while [ $# -gt 0 ]; do

  # printf -- "Loading %-90s" "$(readlink -e "$1") ...";

  source "$(readlink -e "$1")" || {
    printf -- "\nERROR: Failed to import file:\n\n  '%s'\n\n" \
      "$1" >&2;
    exit 1;
  }

  # printf "  [OK]\n";

  shift;
done
