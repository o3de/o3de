#!/bin/bash

# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [[ -h "$SOURCE" ]]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

if [[ "$OSTYPE" = *"darwin"* ]];
then
    PYTHON=$DIR/runtime/python-3.7.10-rev1-darwin/Python.framework/Versions/3.7/bin/python3
else
    PYTHON=$DIR/runtime/python-3.7.10-rev2-linux/python/bin/python
fi

if [[ -e "$PYTHON" ]];
then
    PYTHONNOUSERSITE=1 "$PYTHON" "$@"
    exit $?
fi

echo "Python not found in $DIR"
echo "Try running $DIR/get_python.sh first."
exit 1
