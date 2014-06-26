#!/bin/bash
#  copyHeaders.sh
#
#  Created by damjan stulic on 4/4/14.
#   Optimized by pauley on 6/20/14
#  Copyright (c) 2014 Anki. All rights reserved.

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DSTDIR="$SRCDIR"/../exportedHeaders/lua
#echo src dir = $SRCDIR
#echo dst dir = $DSTDIR
mkdir -p "$DSTDIR"

# copy Lua exported headers
echo "[Copy Lua Exported Headers]"
rsync -r -t -m --chmod=a=r --delete --delete-excluded --files-from="${SRCDIR}"/exportedHeaders.txt "${SRCDIR}"/../lua-5.2.2/src "${DSTDIR}"
if [ $? -ne 0 ]; then
  echo "Error: copying exported headers"
  exit 1;
fi
