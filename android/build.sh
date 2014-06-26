#!/bin/bash

UNITTEST=$1

GIT=`which git`
if [ -z $GIT ]
then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

NDKBUILD=`which ndk-build`
if [ -z $NDKBUILD ]; then
  echo ndk-build not found
  exit 1
fi

"$NDKBUILD" clean

#build Lua
cd "$TOPLEVEL"/android/jni
"$NDKBUILD" -j 7 NDK_DEBUG=1
#$NDKBUILD -j 7 NDK_DEBUG=1 > build.txt 2>&1

#export headers
"$TOPLEVEL"/tools/copyHeaders.sh 
