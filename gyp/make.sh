#!/bin/bash

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SRCDIR

# mac
#$SRCDIR/output/mac/lua.xcodeproj


# ios
#$SRCDIR/output/ios/lua.xcodeproj


# android
cd $SRCDIR/output/lua
ndk-build -j 7