"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

