"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import sys
import azlmbr.bus as bus
import azlmbr.editor as editor

print ('editor command line works')

for x in range(len(sys.argv)):
    print ('editor command line arg {}'.format(sys.argv[x]))

# make sure the @engroot@ exists as a azlmbr.paths property
engroot = azlmbr.paths.engroot
if (engroot is not None and len(engroot) is not 0):
    print ('engroot is {}'.format(engroot))
    print ('editor engroot set')

# make sure the @devroot@ exists as a azlmbr.paths property
devroot = azlmbr.paths.devroot
if (devroot is not None and len(devroot) != 0):
    print ('devroot is {}'.format(devroot))
    print ('editor devroot set')

# resolving a basic path
path = azlmbr.paths.resolve_path('@engroot@/engineassets/texturemsg/defaultsolids.mtl')
if (len(path) != 0 and path.find('@engroot@') == -1):
    print ('path resolved to {}'.format(path))
    print ('path resolved worked')

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
