"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_WaitCommands_Works(BaseClass):
    # Description: 
    # Tests hooks into the timing logic of the Editor
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.legacy.general as general

        preTimePoint = azlmbr.components.TickRequestBus(bus.Broadcast, "GetTimeAtCurrentTick")
        preTimeMs = preTimePoint.GetMilliseconds()

        TICKS_TO_WAIT = 5
        numTicks = 0

        def onTick(args):
            nonlocal numTicks
            numTicks += 1

        handler = bus.NotificationHandler('TickBus')
        handler.connect()
        handler.add_callback('OnTick', onTick)

        general.idle_wait_frames(TICKS_TO_WAIT)
        handler.disconnect()

        postTimePoint = azlmbr.components.TickRequestBus(bus.Broadcast, "GetTimeAtCurrentTick")
        postTimeMs = postTimePoint.GetMilliseconds()

        BaseClass.check_result(postTimeMs > preTimeMs, "Time is bigger after frames")
        BaseClass.check_result(numTicks >= TICKS_TO_WAIT, f"Frames elapsed {numTicks} {TICKS_TO_WAIT}")

if __name__ == "__main__":
    tester = Editor_WaitCommands_Works()
    tester.test_case(tester.test)
