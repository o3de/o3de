"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test Case Title : Verify that a rigid body with "Interpolate motion" option selected moves smoothly.
"""


# fmt: off
class Tests():
    create_entity          = ("Created test entity",                           "Failed to create test entity")
    rigid_body_added       = ("Added PhysX Rigid Body component",              "Failed to add PhysX Rigid Body component")
    enter_game_mode        = ("Entered game mode",                             "Failed to enter game mode")
    exit_game_mode         = ("Exited game mode",                              "Failed to exit game mode")
    rigid_body_smooth      = ("Rigid body motion passed smoothness threshold", "Failed to meet smoothness threshold for rigid body motion")
# fmt: on


def Tick_InterpolatedRigidBodyMotionIsSmooth():
    """
    Summary:
     Create entity with PhysX Rigid Body component and turn on the Interpolate motion setting.
     Verify that the position of the rigid body varies smoothly with time.

    Expected Behavior:
     1) The motion of the rigid body under gravity is a smooth curve, rather than an erratic/jittery movement.

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add rigid body component
     4) Enter game mode and collect data for the rigid body's z co-ordinate and the time values for a series of frames
     5) Check if the motion of the rigid body was sufficiently smooth

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

    # 3) Add rigid body component
    rigid_body_component = test_entity.add_component("PhysX Dynamic Rigid Body")
    rigid_body_component.set_component_property_value("Configuration|Interpolate motion", True)
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearDamping", test_entity.id, 0.0)
    Report.result(Tests.rigid_body_added, test_entity.has_component("PhysX Dynamic Rigid Body"))

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
    Report.result(Tests.rigid_body_smooth, bool(coefficient_of_determination > COEFFICIENT_OF_DETERMINATION_THRESHOLD))


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Tick_InterpolatedRigidBodyMotionIsSmooth)
