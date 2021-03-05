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
# Shell script to update all of the libraries to use rpath to reference the
# Qt libs. This only needs to be run once for the libs in 3rd party so that
# they use the rpath references to the other libraries instead of the hardcoded
# paths when Qt installs them on a machine. This will allow different users to
# use the libraries directly from perforce instead of requiring a local version
# to be installed

DEBUG_PRINT=0
VERBOSE_DEBUG_PRINT=0

runInstallNameToolOn ()
{
    pushd $1/Versions/Current > /dev/null
    libs=$(find . -type f -depth 1)
    for lib in $libs; do
        lib=${lib:2} # Strip off the ./ from the front of the name

        cmd="chmod 666 $lib"
        [ $DEBUG_PRINT -eq 1 ] && printf "\tRunning $cmd\n"
        `$cmd`

        # Don't update the id if it already uses rpath (grep -v returns things
        # that don't match the specified pattern
        if [[ `otool -DX $lib | grep -v @rpath` != "" ]]; then
            cmd="install_name_tool -id @rpath/$1/$lib $lib"
            [ $DEBUG_PRINT -eq 1 ] && printf "\tRunning $cmd\n"
            `$cmd`
        fi

        # only update qt dependencies that don't have an rpath in them
        dependencies=$(otool -LX $lib | cut -d ' ' -f 1 | grep -i qt | grep -v @)
        for qtLib in $dependencies; do
            [ $VERBOSE_DEBUG_PRINT -eq 1 ] && printf "\tTrying to extract lib name from $qtLib\n"
            qtLibName=${qtLib##*/}
            [ $VERBOSE_DEBUG_PRINT -eq 1 ] && printf "\tComparing $qtLibName with $lib\n"
            if [[ $qtLibName != $lib ]]; then
                cmd="install_name_tool -change $qtLib @rpath/$qtLibName.framework/$qtLibName $lib"
                [ $DEBUG_PRINT -eq 1 ] && printf "\tRunning $cmd\n"
                `$cmd`
            fi
        done
    done
    popd > /dev/null
}

if [[ $1 != "" ]]; then
    doPopDir=1
    pushd $1 > /dev/null
fi

frameworksToModify=$(find . -name "*.framework" -type d)
for framework in $frameworksToModify; do
    printf "Looking at $framework...\n"
    runInstallNameToolOn ${framework:2}
done

if [[ $doPopDir -eq 1 ]]; then
    popd > /dev/null
fi
