#!/bin/bash
     
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 
# This script must be run as root
if [[ $EUID -ne 0 ]]
then
    echo "This script must be run as root (sudo)"
    exit 1
fi
 
# Install python3 if necessary
python3 --version >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo Installing Python3
    apt-get install python3
else
    PYTHON_VERSION=$(python3 --version)
    echo Python3 already installed \($PYTHON_VERSION\)
fi
 
# Install python3 pip if necessary
pip3 --version >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo Installing Python3 PIP
    apt-get install -y python3-pip
else
    PYTHON_VERSION=$(pip3 --version | awk '{print $2}')
    echo Python3 Pip already installed \($PYTHON_VERSION\)
fi
 
 
# Read from the package list and process each package
PACKAGE_FILE_LIST=package-list.python3.txt
 
echo Reading python pip package list $PACKAGE_FILE_LIST
 
# Read each line (strip out comment tags)
for LINE in `cat $PACKAGE_FILE_LIST | sed 's/#.*$//g'`
do
    PACKAGE=`echo $LINE | awk -F / '{print $1}'`
    if [ "$PACKAGE" != "" ]  # Skip blank lines
    then
         echo Installing $PACKAGE
         pip3 install $PACKAGE
    fi
done

