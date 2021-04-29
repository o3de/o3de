"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# Test case ID : C24308873
# Test Case Title : Check that cylinder shape collider collides with terrain
# URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/24308873
# A cylinder is suspended slightly over PhysX Terrain to check that it collides when dropped


# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",                "Failed to enter game mode")
    find_cylinder          = ("Cylinder entity found",            "Cylinder entity not found")
    find_terrain           = ("Terrain found",                    "Terrain not found")
    cylinder_above_terrain = ("Cylinder position above ground",   "Cylinder is not above the ground")
    time_out               = ("No time out occurred",             "A time out occurred, please validate level setup")
    touched_ground         = ("Touched ground before time out",   "Did not touch ground before time out")
    exit_game_mode         = ("Exited game mode",                 "Couldn't exit game mode")
# fmt: on


def C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain():
    """
    Summary:
    Runs a test to make sure that a PhysX Rigid Body and Cylinder Shape Collider can successfully collide with a PhysX Terrain entity.

    Level Description:
    A cylinder with rigid body and collider is positioned above a PhysX terrain. 

    Expected Outcome:
    Once game mode is entered, the cylinder should fall toward and collide with the terrain.

    Steps:
    1) Open level and enter game mode
    2) Retrieve entities and positions
    3) Wait for cylinder to collide with terrain OR time out
    4) Exit game mode
    5) Close the editor

    :return:
    """
    import os
    import sys

    import ImportPathHelper as imports

    imports.init()

    import azlmbr.legacy.general as general
    import azlmbr.bus
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Global time out
    TIME_OUT = 1.0

    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities and positions
    cylinder_id = general.find_game_entity("PhysX_Cylinder")
    Report.critical_result(Tests.find_cylinder, cylinder_id.IsValid())

    terrain_id = general.find_game_entity("PhysX_Terrain")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())

    cylinder_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", cylinder_id).GetPosition()
    terrain_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", terrain_id).GetPosition()
    Report.info_vector3(cylinder_pos, "Cylinder:")
    Report.info_vector3(terrain_pos, "Terrain:")
    Report.critical_result(
        Tests.cylinder_above_terrain,
        (cylinder_pos.z - terrain_pos.z) > 0.5,
        "Please make sure the cylinder entity is set above the terrain",
    )

    # Enable gravity (just in case it is not enabled)
    if not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", cylinder_id):
        azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", cylinder_id, True)

    class TouchGround:
        value = False

    # Collision event handler
    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(terrain_id):
            Report.info("Touched ground")
            TouchGround.value = True

    # Assign event handler to cylinder
    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(cylinder_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    # 3) Wait for the cylinder to hit the ground OR time out
    test_completed = helper.wait_for_condition((lambda: TouchGround.value), TIME_OUT)

    Report.critical_result(Tests.time_out, test_completed)
    Report.result(Tests.touched_ground, TouchGround.value)

    # 4) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from editor_python_test_tools.utils import Report
    Report.start_test(C24308873_CylinderShapeCollider_CollidesWithPhysXTerrain)
