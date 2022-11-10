#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

SOURCE="${BASH_SOURCE[0]}"
# While $SOURCE is a symlink, resolve it
while [[ -h "$SOURCE" ]]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
    SOURCE="$( readlink "$SOURCE" )"
    # If $SOURCE was a relative symlink (so no "/" as prefix, need to resolve it relative to the symlink base directory
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

if [[ "$OSTYPE" == *"darwin"* ]]; then
    PYTHON=$DIR/../../../../runtime/python-3.10.5-rev2-darwin/Python.framework/Versions/3.10/bin/python3
elif [[ "$OSTYPE" == "msys" ]]; then
    PYTHON=$DIR/../../../../runtime/python-3.10.5-rev1-windows/python/python.exe
else
    PYTHON=$DIR/../../../../runtime/python-3.10.5-rev2-linux/python/bin/python
fi

if [[ -e "$PYTHON" ]];
then
    PYTHONNOUSERSITE=1 "$PYTHON" "$@"
    exit $?
fi

echo "Python not found in $DIR/../../../.."
echo "Try running $DIR/../../../../get_python.sh first."
exit 1
