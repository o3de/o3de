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
# Shell script that updates the Qt libraries and executables that are in the
# 3rd party directory so that they contain the correct rpath references so that
# the Qt command line executables can run properly as well as executables
# linking to the libraries will automatically get the rpath versions and no
# post build processing on them will need to be done.

DEBUG_PRINT=1

qtBaseDir="$1"

# Try and guess where 3rd party is located by seeing if we can sneak our way
# through the backdoor symbolic link in the sandbox
relativePathToSandboxSDKFromScriptLocaiton="../../Sandbox/SDKs/Qt"
if [[ $qtBaseDir == "" && -e $relativePathToSandboxSDKFromScriptLocaiton ]]; then
    pushd $relativePathToSandboxSDKFromScriptLocaiton > /dev/null
    qtBaseDir=`pwd`
    popd > /dev/null
fi

shopt -s nocasematch

if [[ $qtBaseDir != "" ]]; then
    read -e -p "Use the following Qt directory: $qtBaseDir? [y/n] " useQtBaseDir
fi

if [[ $qtBaseDir == "" || $useQtBaseDir == "n" ]]; then
    read -e -p "Please enter the location of the Qt directory in the 3rd party directory: " qtBaseDir
fi

[ $DEBUG_PRINT -eq 1 ] && echo "qtBaseDir = *$qtBaseDir*"

#(exec ./printQtLibDependencies.sh $qtBaseDir)
echo "Updating Qt libraries..."
(exec ./updateQtLibs.sh $qtBaseDir/lib)
echo "Updating Qt executables..."
(exec ./updateQtExecs.sh $qtBaseDir/bin)
