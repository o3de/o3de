"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#
# testing Python code
# 
import sys
import os
import os.path

print('EditorPythonBindingsTestWithArgs_RunScriptFile')
print('num args: {}'.format(len(sys.argv)))

# Intentionally print script name separately from the other args.
# The path that it prints will be non-deterministic based on where the code
# has been synced to, so we strip it off, enabling us to just validate the script name
# and the other args made it through successfully.
print('script name: {}'.format(os.path.basename(sys.argv[0])))
for arg in range(1, len(sys.argv)):
    print('arg {}: {}'.format(arg, sys.argv[arg]))
