"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_WaitCommands_Works():
    # Description: 
    # Tests hooks into the timing logic of the Editor

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.math
    import azlmbr.legacy.general as general

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

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

    check_result(postTimeMs > preTimeMs, "Time is bigger after frames")
    check_result(numTicks >= TICKS_TO_WAIT, f"Frames elapsed {numTicks} {TICKS_TO_WAIT}")

    # all tests worked
    Report.result("Editor_WaitCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_WaitCommands_Works)





