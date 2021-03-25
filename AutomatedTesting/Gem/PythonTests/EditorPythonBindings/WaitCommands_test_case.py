"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math
import azlmbr.legacy.general as general

# open any level
general.idle_enable(True)
general.idle_wait(0.5)

preTimePoint = azlmbr.components.TickRequestBus(bus.Broadcast, "GetTimeAtCurrentTick")
preTimeMs = preTimePoint.GetMilliseconds()

TICKS_TO_WAIT = 5

numTicks = 0


def onTick(args):
    global numTicks
    numTicks += 1


handler = bus.NotificationHandler('TickBus')
handler.connect(None)
handler.add_callback('OnTick', onTick)

general.idle_wait_frames(TICKS_TO_WAIT)

postTimePoint = azlmbr.components.TickRequestBus(bus.Broadcast, "GetTimeAtCurrentTick")
postTimeMs = postTimePoint.GetMilliseconds()

if postTimeMs > preTimeMs:
    print("Time is bigger after frames")

if numTicks == TICKS_TO_WAIT:
    print("Exact frames elapsed")

handler.disconnect()

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
