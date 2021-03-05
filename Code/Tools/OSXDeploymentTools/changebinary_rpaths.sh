#!/bin/bash
# 
# 
#  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
#  its licensors.
# 
#  For complete copyright and license terms please see the LICENSE at the root of this
#  distribution (the "License"). All use of this software is governed by the License,
#  or, if provided, by the license below or the license accompanying this file. Do not
#  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# 
# this script modifies a single app, passed in, on the command line, to use @rpath dirs for Qt libs
# you are expected to pass in the executable name on the command line as the first argument
# you are expected to set QTDIR.
# it works by searching all frameworks in QtDir/lib and seeing if your app has a reference to it
# if it does, it replaces that reference with a relative-path reference instead.

NEWLIBPATH="@rpath"
ORIGINALLIBPATH=${QTDIR}/lib

if [ -z "$1" ]; then
echo "Please input the application file to modify"
exit 1
fi

if [ -z "${QTDIR}" ]; then
echo "QTDIR IS NOT SET.  PLease set QTDIR to the root of your Qt platform install, usually Qt/VersionNumber/clang_64"
exit 1
fi

if [ ! -d "$QTDIR" ]; then
echo "QTDIR does not appear to point at a directory ,currently its set to ${QTDIR}"
exit 1
fi

echo changebinary_rpaths.sh: Remapping paths in $1 to be rpath instead.

# for each original framework, amend all frameworks in the target folder:
ORIGINAL_LIBS=${ORIGINALLIBPATH}/*.framework
for ORIGINAL_LIB in ${ORIGINAL_LIBS} ; do
BIN_BASE_NAME=$(basename "${ORIGINALLIBPATH}/${ORIGINAL_LIB}" .framework)
ORIGINAL_BINARY_NAME=`otool -DX "${ORIGINALLIBPATH}/${BIN_BASE_NAME}.framework/${BIN_BASE_NAME}"`
NEWTARGETID=${NEWLIBPATH}/${BIN_BASE_NAME}.framework/${BIN_BASE_NAME}

# uncomment to debug:
echo Remapping ${ORIGINAL_BINARY_NAME} ==\> ${NEWTARGETID}
# BIN_BASE_NAME now contains the name of the lib, like 'QtCore'
# ORIGINAL_BINARY_NAME now contains the full path to the binary inside the framework
# for example /Users/lawsonn/Qt/5.3/clang_64/lib/Qt5Core.framework/Versions/5/Qt5Core
# NEWTARGETID is the one to change it to
# for example, @rpath/QtCore.framework/QtCore

# replace it in the executable:
install_name_tool -change "${ORIGINAL_BINARY_NAME}" "${NEWTARGETID}" "$1"
done

# now, we need to rename the dylib's own ID to be @rpath<currentId>
MY_NAME=`otool -DX "$1"`
install_name_tool -id "${NEWLIBPATH}/${MY_NAME}" "$1"
