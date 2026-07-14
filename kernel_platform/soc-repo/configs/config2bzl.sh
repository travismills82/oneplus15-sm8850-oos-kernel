#SPDX-License-Identifier: GPL-2.0-only
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.

#!/usr/bin/env bash

convert() {
	echo "$(basename $1 | sed -e 's/\./_/g') = {"
	echo "    # keep sorted"
	sed -e 's/\(CONFIG_.*\)="\([^"]*\)"/    "\1": "\\"\2\\"",/' -e 's/\(CONFIG_.*\)=\([^"]*\)/    "\1": "\2",/' -e 's/# \(CONFIG_.*\) is not set/    "\1": "n",/' $1
	echo "}"
}

if which buildifier > /dev/null ; then
	convert $* | buildifier -lint=fix -warnings=all
else
	convert $*
fi
