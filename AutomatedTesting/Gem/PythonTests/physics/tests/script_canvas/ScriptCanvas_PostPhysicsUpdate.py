"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C14902098
# Test Case Title : Check that force region simulation with Postsimulate works independently from rendering tick



# fmt: off
class Tests():
    enter_game_mode   = ("Entered game mode",     "Failed to enter game mode")
    find_sphere       = ("Sphere found",          "Sphere not found")
    find_force_region = ("Force Region is found", "Force Region is not found")
    exit_game_mode    = ("Exited game mode",      "Couldn't exit game mode")
# fmt: on


class LogLines:
    """
    These lines are added to the expected lines in the test suite.
    """

    expected_lines = [
        "OnTick Event: The Sphere position did not change from previous position",
        "OnTick Event: The Sphere position changed from previous position",
        "OnPostPhysicsSubtick Event: The Sphere position changed from previous position",
    ]
    unexpected_lines = ["OnPostPhysicsSubtick Event: The Sphere position did not change from previous position"]


def ScriptCanvas_PostPhysicsUpdate():

    """
    Summary:
    Check that force region simulation with Postsimulate works independently from rendering tick.

    Level Description:
    A Sphere is placed inside a Force Region. The "Fixed Time Step" in PhysX Configuration is set to 0.05.
    ForceRegion (entity) - Entity with PhysX Collider, PhysX Force Region (World Space force with magnitude 5.0) and
                           Box Shape components
    Sphere (entity) - Entity with PhysX Rigid Body, PhysX Collider, Mesh and 2 Script Canvas components
    Script Canvas:
    onpostphysicsupdate - The script checks the position of the sphere on every On Postsimulate event and prints
                          debug statements as per the position of the sphere relative to its previous position.
    ontick - The script checks the position of the sphere on every On Tick event and prints
             debug statements as per the position of the sphere relative to its previous position.

    Expected Behavior:
    The position of the sphere needs to be changed relative to its previous position for every
    OnPostPhysicsSubtick event.
    The position of the sphere sometimes change and sometimes remains in the same position as before
    for OnTick event.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Wait for WAIT_TIME for the events to occur
     5) Exit game mode
     6) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general

    # Constants
    WAIT_TIME = 0.5

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "ScriptCanvas_PostPhysicsUpdate")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    sphere_id = general.find_game_entity("Sphere")
    Report.critical_result(Tests.find_sphere, sphere_id.IsValid())
    force_region_id = general.find_game_entity("ForceRegion")
    Report.critical_result(Tests.find_force_region, force_region_id.IsValid())

    # 4) Wait for WAIT_TIME for the events to occur
    general.idle_wait(WAIT_TIME)

    # 5) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_PostPhysicsUpdate)
