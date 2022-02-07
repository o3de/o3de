"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# LY-107818 reported a crash with this code snippet
# This new test will be used to regress test the issue

import azlmbr.bus as bus
import azlmbr.editor as editor

try:
    componentList = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameList')
    if(componentList is not None and len(componentList) > 0):
        print("BuildComponentTypeNameList returned a valid list: SUCCESS")

    print("BuildComponentTypeNameList ran: SUCCESS")
except:
    print("BuildComponentTypeNameList usage threw an exception: FAILURE")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')

