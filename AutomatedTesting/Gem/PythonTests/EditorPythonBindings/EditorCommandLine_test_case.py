"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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

# resolving a basic path
path = azlmbr.paths.resolve_path('@engroot@/engineassets/texturemsg/defaultsolids.mtl')
if (len(path) != 0 and path.find('@engroot@') == -1):
    print ('path resolved to {}'.format(path))
    print ('path resolved worked')

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
