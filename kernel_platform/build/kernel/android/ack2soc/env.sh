#!/bin/bash
# Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

export KERNEL_PLATFORM_ROOT;

export ACK_URL;
export ACK_BRANCH_BASENAME;
export ACK_BRANCH;
export LTS_BRANCH;

# This is the SHA, which will be scanned for new changes. On LTS import this is
# the SHA being imported i.e. LTS manifest change reset common folder to this
# SHA.
export ACK_SHA;

# This is the SHA on which is reset $ACK_ROOT (common) folder i.e. this was
# already imported into kernel platform (KP) via some previous LTS.
export ACK_SHA_KP;
export ACK_REPO_REL_ROOT;
export ACK_ROOT;

export SOC_URL;
export SOC_BRANCH;
export SOC_SHA;
export SOC_REPO_REL_ROOT;
export SOC_ROOT;

export LIST_DRIVERS;

export DRIVER_MARKER;
export DEVICETREE_BINDINGS_PATH;

export REPORT_STALLED=false;
export     NOT_UPDATED_WARNING_PERIOD;
declare -i NOT_UPDATED_WARNING_PERIOD;
export REPORT_ALL=false;

export -a CHANGES=();
export -A CHANGES_ALL=();

export INTERACTIVE;
export PROGRESS;
export DEBUG;

export TIMESTAMP;
export DEBUG_LOG;

# List of keywords which are matching to list of files and directories
DRIVERS=();
PATHS=();

# Files to process. This is prepared from the above arrays. These are the files
# for which we would like to list/pick the changes from upstream (ACK_SHA).
FILES=();

ACK_URL="${ACK_URL:-ssh://review-android.quicinc.com:29418/kernel/common}";
ACK_BRANCH_BASENAME="${ACK_BRANCH_BASENAME:-android16-6.12}";
ACK_BRANCH="${ACK_BRANCH:-refs/heads/aosp/$ACK_BRANCH_BASENAME}";
ACK_REPO_REL_ROOT="${ACK_REPO_REL_ROOT:-kernel_platform/common}";

SOC_URL="${SOC_URL:-ssh://review-android.quicinc.com:29418/kernel/qcom}";
SOC_BRANCH="${SOC_BRANCH:-refs/heads/presil-canoe}";
SOC_REPO_REL_ROOT="${SOC_REPO_REL_ROOT:-kernel_platform/soc-repo}";

# Existence of this file marks the containing folder as a driver folder
DRIVER_MARKER='modules.bzl';
DEVICETREE_BINDINGS_PATH="${DEVICETREE_BINDINGS_PATH:-Documentation/devicetree/bindings}";

NOT_UPDATED_WARNING_PERIOD=60 # days

# Any delta done by an ACK change in below path prefixes, will be NOT taken
# into SOC repo.
BLOCKLIST=(
  "$DEVICETREE_BINDINGS_PATH"
  'scripts'
  'arch/arm64/boot/dts'
  'BUILD.bazel'
  'arch/arm64/configs/consolidate.fragment'
  'arch/arm64/configs/generic_vm_defconfig'
);
BLOCKLIST_REGEXES=();

TIMESTAMP="$(date +"%Y-%m-%d_%H.%M.%S")";

DEBUG="${DEBUG:-false}";
INTERACTIVE="${INTERACTIVE:-false}";
PROGRESS="${PROGRESS:-false}";

LIST_DRIVERS='false';

DELETE_QUEUE=();
