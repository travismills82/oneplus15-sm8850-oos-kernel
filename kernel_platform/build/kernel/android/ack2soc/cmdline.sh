#!/bin/bash
# Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: GPL-2.0-only

LONG_OPTIONS_LIST=(
  'driver:'
  'list-drivers'
  'stalled::'
  'report-all'
  'kp-root:'
  'kernel-platform-root:'
  'soc-root:'
  'soc-url:'
  'soc-branch:'
  'soc-sha:'
  'ack-root:'
  'ack-url:'
  'ack-branch:'
  'ack-sha-kp:'
  'ack-sha:'
  'interactive'
  'progress'
  'dry-run'
  'help'
  'verbose'
);
SHORT_OPTIONS_LIST='d: l h i v';

function usage()
{
  cat <<USAGE ;

USAGE: $(basename "$0") [OPTIONS] PATH ...

DESCRIPTION:
    This script compares upstream drivers|files in SOC Repo against ones in
    Common Kernel and generate list of missing commits. Free arguments are
    files|dirs to process. Home page is go/socrepo.

    Get list of all changes, between two ACK SHAs, which are touching ONLY the
    common files between SOC and the left ACK SHA, ordered in reverse by the
    time of their appearance in the history limited by these two ACK SHAs.

OPTIONS:
  -d | --driver
           This is driver string, which is expanded to list of files. You could
           check the list of all drivers by run the script with option
           \`--list-drivers'. Driver string is a path in the SOC repo, it is a
           shortcut to list of all files located under that directory.

  -l | --list-drivers
           List all available drivers loaded in SOC repo

       --stalled[=days]
           Report files that are not being updated for more than specified days.
           If no value is provided default is $NOT_UPDATED_WARNING_PERIOD days.

       --report-all
           Along to modified files, which we care, shows also files which are
           new/deleted in SOC compared to ACK.

       --kp-root, --kernel-platform-root
           [default: $(readlink -m "$KERNEL_PLATFORM_ROOT")]
           Set root of the KERNEL PLATFORM workspace. This is a shortcut to set
           roots to SOC and ACK repos in the workspace as follows:
             ACK: \${KERNEL_PLATFORM_ROOT}/$ACK_REPO_REL_ROOT
             SOC: \${KERNEL_PLATFORM_ROOT}/$SOC_REPO_REL_ROOT

       --soc-root
           [default: $(readlink -m "$SOC_ROOT")]
           Root folder of the SOC repo (trimmed MSM kernel)

       --soc-url
           [default: $SOC_URL]

       --soc-branch
           [default: $SOC_BRANCH]
           Name of the SOC branch, which will receive the changes

       --soc-sha
           [default: $SOC_SHA]
           SHA (commit id) to base the cherry-picks

       --ack-root
           [default: $(readlink -m "$ACK_ROOT")]
           Root folder of the ACK repo (AOSP Common Kernel)

       --ack-url
           [default: $ACK_URL]
           URL of the ACK kernel (Q: is it more ok to use up|upstream instead
           ACK)

       --ack-branch
           [default: $ACK_BRANCH]
           Intake branch. This branch will be scanned for incoming changes

       --ack-sha-kp
           [default: $ACK_SHA_KP]
           SHA (commit id) from ACK, which is already merged into kernel
           platform (KP) - needs to be in sync with SOC SHA i.e. most latest
           ACK SHA that is part of SOC SHA history OR in a freshly init/sync of
           KP workspace it is the top commit in common folder:

             <KP_ROOT>/kernel_platform/common

       --ack-sha
           Provide the SHA from ACK (intake) upstream branch to check for
           missing changes.

       --interactive
           Prompts the user during execution of the script.

       --progress
           For actions that are taking longer, shows info to indicating that
           processing is ongoing and not stuck, instead of just sitting and not
           printing anything.

       --dry-run
           Run script in debug mode. Show what commands will execute. NOTE that
           there  are  NO  guarantee  that  it will be accurate. Do not rely
           blindly,  just  use  it as a reference, merely what commands will be
           executed.

  -h | --help
           Print this help message.

  -v | --verbose
          Be verbose i.e. print more useful info.

CONCEPT:

    The logic requires three SHAs:

      ACK_SHA
        Provided via --ack-sha option. Usually the corresponding one to the
        current LTS that is being imported.

      ACK_SHA_KP
        Provided via --ack-sha-kp option. Usually the latest ACK_SHA that was
        integrated. If not provided, it is obtained from the tip of:

          \${KERNEL_PLATFORM_ROOT}/$ACK_REPO_REL_ROOT

      SOC_SHA
        Provided via --soc-sha option. Usually the SOC repo SHA matching the tip
        of KP N.M. If not provided, it is obtained from the tip of:

          \${KERNEL_PLATFORM_ROOT}/$SOC_REPO_REL_ROOT

    It prepares a list of common FILES between SOC_SHA and ACK_SHA_KP reduced
    with files matched against BLOCKLIST array of patterns:

$(
    printf -- "    %s\n" "${BLOCKLIST[@]}";
)

    Get list of ALL changes, between ACK_SHA_KP and ACK_SHA, which are touching
    only the FILES ordered in reverse by the time of their appearance in the
    history limited by ACK_SHA_KP and ACK_SHA.

    The workspace is enough to have _only_ SOC repo and ACK folders, which is
    not necessary to be part from KP M.N workspace they can be isolated folders
    and their paths can be set via --soc-root and --ack-root options. Here is a
    quick way to setup the workspace:

      Workspace
      ---------
      repo init                                                    \\
        --repo-url=ssh://git.quicinc.com:29418/tools/repo.git      \\
        --repo-branch=qc/stable                                    \\
        -u ssh://git.quicinc.com:29418/kernelplatform/manifest.git \\
        -b KERNEL.PLATFORM.N.M

      repo sync                                       \\
        --current-branch                              \\
        --no-tags                                     \\
        --jobs 48                                     \\
        kernel_platform/{common,soc-repo,build/kernel};

      N >= 5, N=0; M >= 0

      NOTE: Script itself can be found in
            \${KERNEL_PLATFORM_ROOT}/build/kernel

EXAMPLES:

    # Print list of all (valid) driver identifiers
    $(basename "$0") -l;

    $(basename "$0") --driver drivers/ufs/host [--driver DRIVER_PATH ...];

    export SOC_WORKSPACE_ROOT=/path/to/kernel_platform/root;
    $(basename "$0")                                                    \\
      --soc-root "\${SOC_WORKSPACE_ROOT}/kernel_platform/soc-repo" \\
      --progress                                                  \\
      --report-all                                                \\
      --ack-sha 544ae1decd                                        \\
      --soc-sha b25280fac9                                        \\
      --stalled;

USAGE

  return 0;
}

function process_options_ack2soc()
{
  local i;

  while [ $# -ge 1 ]; do
    i="$1"; shift 1;
    $DEBUG && echo "Process option: $i";
    case "$i" in
      --driver|-d)
        # Later this array will be replaced by expanded list of files by
        # using files() function
        DRIVERS+=("$1"); shift 1;
      ;;
      -l|--list-drivers)
        LIST_DRIVERS=true;
      ;;
      --stalled)
        REPORT_STALLED=true;
        NOT_UPDATED_WARNING_PERIOD="${1:-$NOT_UPDATED_WARNING_PERIOD}";
        shift 1;
      ;;
      --report-all)
        REPORT_ALL=true;
      ;;
      --kp-root|--kernel-platform-root)
        KERNEL_PLATFORM_ROOT="$1"; shift 1;
      ;;
      --soc-root)
        SOC_ROOT="$1"; shift 1;
      ;;
      --soc-url)
        SOC_URL="$1"; shift 1;
      ;;
      --soc-branch)
        SOC_BRANCH="$1"; shift 1;
      ;;
      --soc-sha)
        SOC_SHA="$1"; shift 1;
      ;;
      --ack-root)
        ACK_ROOT="$1"; shift 1;
      ;;
      --ack-url)
        ACK_URL="$1"; shift 1;
      ;;
      --ack-branch)
        ACK_BRANCH="$1"; shift 1;
      ;;
      --ack-sha-kp)
        ACK_SHA_KP="$1"; shift 1;
      ;;
      --ack-sha)
        ACK_SHA="$1"; shift 1;
      ;;
      -i|--interactive)
        INTERACTIVE=true;
      ;;
      --progress)
        PROGRESS=true;
      ;;
      --dry-run)
        DRY_RUN=true;
      ;;
      --help|-h)
        return 2;
      ;;
      -v|--verbose)
        VERBOSE=true;
      ;;
      # End of options portion, stop parsing anymore. Remain argumenst are
      # not options, but free arguments
      --)
        break;
      ;;
      # Getopt should discard unknown options. This is just precaution and
      # paranoia
      -*)
        printf "Unknown option: $i\n" >&2;
        return 3;
      ;;
    esac
  done

  # Any remaining free arguments are paths to process
  PATHS=("$@");

  return 0;
}

#
# Test if getopt is enhanced version. This version can handle
# commandline arguments, which contains spaces
#
function enhanced_getopt_check()
{
  getopt -T >/dev/null 2>&1;

  if [ $? -ne 4 ]; then
      cat <<WARN ;
WARNING: you do not have enhanced getopt, so you can't use spaces inside
WARNING: arguments and option values, even you enclose them in quotes
WARN
      $INTERACTIVE && read -p "Press [ENTER] to continue";
  fi >&2
}

function getopt_status()
{
  local status="$1"; shift 1;

  case $status in
    0) : # no errors
      ;;
    1)
      echo "ERROR: getopt: invalid options/arguments detected";
      ;;
    2|3)
      echo "ERROR: getopt: internal error";
      ;;
    *)
      status=4;
      echo "ERROR: getopt: unknown error";
      ;;
  esac

  return $status;
}

function parse_cmdline()
{
  local process_options_handler="$1"; shift 1;

  local cmdline_errors;
  local -i status=0;
  local args='';

  enhanced_getopt_check;

  cmdline_errors="$(_mktemp "${process_options_handler}_errors")";
  DELETE_QUEUE+=("$cmdline_errors");

  args="$(
    $DEBUG && set -x;
    getopt                                    \
      --options "$SHORT_OPTIONS_LIST"         \
      --longoptions "${LONG_OPTIONS_LIST[*]}" \
      --                                      \
      "$@"                                    \
      2>"$cmdline_errors";
  )" || status="$?";

  if [ $status -eq 0 ]; then
    # This will take effect only for positional parameters to this function
    if eval set -- $args; then
      "$process_options_handler" "$@" 2>>"$cmdline_errors";
      status="$?";
    fi
  else
    getopt_status "$status" 1>>"$cmdline_errors";
    status=1; # means that getopt has failed
  fi

  if [[ $status -ne 0 ]]; then
    set-defaults;
    if [[ $status -eq 2 ]]; then
      # Show help on pages if -h|--help is provided
      usage | less >&2;
      exit 0;
    else
      usage >&2;
    fi
    cat "$cmdline_errors" >&2;
  fi


  return "$status";
}
