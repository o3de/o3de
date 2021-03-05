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
# Shell script to update all of the executables to use rpath to reference the
# Qt libs that are stored in 3rd party. Not doing this prevents the executables
# from running on the mac since they will have embedded in them the full
# pathname of the QtLib where it was installed on the person's maching that
# copied the libs into 3rd party. (use otool -LX [exe name] to find out the
# names)

DEBUG_PRINT=0
VERBOSE_DEBUG_PRINT=0

runInstallNameToolOn ()
{
    # figure out how far back we need to go to get to the Qt/lib dir where the
    # frameworks are and where rpath will be pointing to
    IFS=/
    backDirString=""
    for dir in $1; do
        [ $VERBOSE_DEBUG_PRINT -eq 1 ] && printf "testing $dir length: ${#dir}\n"
        backDirString=$backDirString"/.."
    done
    unset IFS

    [ $VERBOSE_DEBUG_PRINT -eq 1 ] && printf "backDirString = $backDirString\n"
    pushd $1 > /dev/null
    executables=$(find . -type f -d 1 -perm +1)
    for file in $executables; do
        file=${file:2} # strip ./ from the front of the file name
        # Get all the Qt libraries that don't have an rpath in them
        dependencies=$(otool -LX $file | cut -d ' ' -f 1 | grep -i qt | grep -v @rpath)
        if [[ ${#dependencies} -gt 0 ]]; then
            [ $DEBUG_PRINT -eq 1 ] && printf "\tUpdating $file to use rpath and relative qt libs\n"

            cmd="chmod 777 $file"
            [ $DEBUG_PRINT -eq 1 ] && printf "\tRunning $cmd\n"
            `$cmd`

            cmd="install_name_tool -add_rpath @executable_path$backDirString/lib $file"
            [ $DEBUG_PRINT -eq 1 ] && printf "\tRunning $cmd\n"
            `$cmd`
        fi
        for qtLib in $dependencies; do
            qtLibName=${qtLib##*/}
            cmd="install_name_tool -change $qtLib @rpath/$qtLibName.framework/$qtLibName $file"
            [ $DEBUG_PRINT -eq 1 ] && printf "\tRunning $cmd\n"
            `$cmd`
        done
    done
    popd > /dev/null
}

if [[ $1 != "" ]]; then
    doPopDir=1
    pushd $1 > /dev/null
fi

directoriesToLookIn=$(find . -type d)
for dir in $directoriesToLookIn; do
    printf "Looking in $dir for Qt executables...\n"
    runInstallNameToolOn ${dir}
done

if [[ $doPopDir -eq 1 ]]; then
    popd > /dev/null
fi
