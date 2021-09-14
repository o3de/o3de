"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C13351703
# Test Case Title : Check that Center of Mass calculations should not include trigger shapes



# fmt: off
class Tests():
    enter_game_mode = ("Entered game mode",                    "Failed to enter game mode")
    find_entities   = ("Entities are found",                   "Entities are not found")
    com_expected    = ("COM value is equal to expected value", "COM value is not equal to expected value")
    exit_game_mode  = ("Exited game mode",                     "Couldn't exit game mode")
# fmt: on


def RigidBody_COM_NotIncludesTriggerShapes():

    """
    Summary:
    Check that Center of Mass calculations should not include trigger shapes.

    Level Description:
    RigidBody (entity) - Entity with 1 Rigid Body component and 2 PhysX Collider components
                         Rigid Body Component - Debug Draw Collider, Compute COM are enabled, Gravity is disabled
                         1st Collider - Offset(-1.0, 0.0, 0.0) - Trigger enabled
                         2nd Collider - Offset(1.0, 0.0, 0.0) - Trigger disabled

    Expected Behavior:
    We are checking if the entity is valid.
    We are verifying if the center of mass is close to the collider whose trigger has been disabled.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Validate if COM is same as expected value
     5) Exit game mode
     6) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test critical_results.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.math as lymath

    # Constants
    OFFSET = lymath.Vector3(1.0, 0.0, 0.0) # Offset of the trigger disabled sphere
    CLOSE_THRESHOLD = sys.float_info.epsilon

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_COM_NotIncludesTriggerShapes")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    rigid_body_id = general.find_game_entity("RigidBody")
    Report.critical_result(Tests.find_entities, rigid_body_id.IsValid())

    # 4) Validate if COM is same as expected value
    entity_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", rigid_body_id)
    com = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetCenterOfMassWorld", rigid_body_id)
    Report.result(Tests.com_expected, entity_position.Add(OFFSET).IsClose(com, CLOSE_THRESHOLD))

    # 5) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_COM_NotIncludesTriggerShapes)
