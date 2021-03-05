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
# Shell script to print out the various qt libraries that a Qt framework
# depends on. The libs that do not have @rpath in their name will need to
# either be copied into the app bundle and be updated to use rpath or be
# updated so that when an executable linking against them will pull in the
# rpath information and use that instead of the hardcoded lib id. The
# updateLibs.sh script will perform the necessary operations to update the lib
# to have an rpath ID and change the name of the dynamic Qt libs it depends on
# to use the rpath reference instead of a absolute path

DEBUG_PRINT=1
VERBOSE_DEBUG_PRINT=0

runInstallNameToolOn ()
{
    pushd $1/Versions/Current > /dev/null
    libs=$(find . -type f -depth 1)
    for lib in $libs; do
        lib=${lib:2} # Strip off the ./ from the front of the name
        printf "\t$lib has these dependenices\n"
        dependencies=$(otool -LX $lib | cut -d ' ' -f 1 | grep -i qt)
        for qtLib in $dependencies; do
            qtLibName=${qtLib##*/}
            [ $qtLibName != $lib ] && printf "\t\t$qtLib\n"
        done
    done
    popd > /dev/null
}

if [[ $1 != "" ]]; then
    doPopDir=1
    pushd $1 > /dev/null
fi

frameworks=$(find . -name "*.framework" -type d)
for framework in $frameworks; do
    printf "$framework\n"
    runInstallNameToolOn ${framework:2}
done

if [[ $doPopDir -eq 1 ]]; then
    popd > /dev/null
fi
