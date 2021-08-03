"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
