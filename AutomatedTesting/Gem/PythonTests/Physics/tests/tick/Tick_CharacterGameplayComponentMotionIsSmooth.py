"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test Case Title : Verify that an entity with a character gameplay component moves smoothly.
"""


# fmt: off
class Tests():
    create_entity              = ("Created test entity",                          "Failed to create test entity")
    character_controller_added = ("Added PhysX Character Controller component",   "Failed to add PhysX Character Controller component")
    character_gameplay_added   = ("Added PhysX Character Gameplay component",     "Failed to add PhysX Character Gameplay component")
    enter_game_mode            = ("Entered game mode",                            "Failed to enter game mode")
    exit_game_mode             = ("Exited game mode",                             "Failed to exit game mode")
    character_motion_smooth    = ("Character motion passed smoothness threshold", "Failed to meet smoothness threshold for character motion")
# fmt: on


def Tick_CharacterGameplayComponentMotionIsSmooth():
    """
    Summary:
     Create entity with PhysX Character Controller and PhysX Character Gameplay components.
     Verify that the motion of the character controller under gravity is smooth.

    Expected Behavior:
     1) The motion of the character controller under gravity is a smooth curve, rather than an erratic/jittery movement.

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add a PhysX Character Controller Component and PhysX Character Gameplay component
     4) Enter game mode and collect data for the character controller's z co-ordinate and the time values for a series of frames
     5) Check if the motion of the character controller was sufficiently smooth

    :return: None
    """
    # imports
    import os
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.asset_utils import Asset
    import numpy as np

    # constants
    COEFFICIENT_OF_DETERMINATION_THRESHOLD = 1 - 1e-4 # curves with values below this are not considered sufficiently smooth

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("", "Base")

    # 2) Create an entity
    test_entity = Entity.create_editor_entity("test_entity")
    Report.result(Tests.create_entity, test_entity.id.IsValid())

    azlmbr.components.TransformBus(
        azlmbr.bus.Event, "SetWorldTranslation", test_entity.id, math.Vector3(0.0, 0.0, 0.0))

    # 3) Add character controller and character gameplay components
    character_controller_component = test_entity.add_component("PhysX Character Controller")
    Report.result(Tests.character_controller_added, test_entity.has_component("PhysX Character Controller"))
    character_gameplay_component = test_entity.add_component("PhysX Character Gameplay")
    Report.result(Tests.character_gameplay_added, test_entity.has_component("PhysX Character Gameplay"))

    # 4) Enter game mode and collect data for the rigid body's z co-ordinate and the time values for a series of frames
    t = []
    z = []
    helper.enter_game_mode(Tests.enter_game_mode)
    general.idle_wait_frames(1)
    game_entity_id = general.find_game_entity("test_entity")
    for frame in range(100):
        t.append(azlmbr.components.TickRequestBus(azlmbr.bus.Broadcast, "GetTimeAtCurrentTick").GetSeconds())
        z.append(azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", game_entity_id))
        general.idle_wait_frames(1)
    helper.exit_game_mode(Tests.exit_game_mode)

    # 5) Test that the z vs t curve is sufficiently smooth (if the interpolation is not working well, the curve will be less smooth)
    # normalize the t and z data
    t = np.array(t) - np.mean(t)
    z = np.array(z) - np.mean(z)
    # fit a polynomial to the z vs t curve
    fit = np.poly1d(np.polyfit(t, z, 4))
    residual = fit(t) - z
    # calculate the coefficient of determination (a measure of how closely the polynomial curve fits the data)
    # if the coefficient is very close to 1, then the curve fits the data very well, suggesting that the rigid body motion is smooth
    # if the coefficient is significantly less than 1, then the z values vary more erratically relative to the smooth curve,
    # indicating that the motion of the rigid body is not smooth
    coefficient_of_determination = (1 - np.sum(residual * residual) / np.sum(z * z))
    Report.result(Tests.character_motion_smooth, bool(coefficient_of_determination > COEFFICIENT_OF_DETERMINATION_THRESHOLD))


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Tick_CharacterGameplayComponentMotionIsSmooth)
