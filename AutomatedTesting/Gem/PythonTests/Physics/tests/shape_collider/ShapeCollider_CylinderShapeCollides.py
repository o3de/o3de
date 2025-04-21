"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C24308873
# Test Case Title : Check that cylinder shape collider collides with terrain

# A cylinder is suspended slightly over PhysX Terrain to check that it collides when dropped


# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",                "Failed to enter game mode")
    find_cylinder          = ("Cylinder entity found",            "Cylinder entity not found")
    create_terrain         = ("Terrain entity created successfully", "Failed to create Terrain Entity")
    find_terrain           = ("Terrain entity found",            "Terrain entity not found")
    add_physx_shape_collider = ("Added PhysX Shape Collider",          "Failed to add PhysX Shape Collider")
    add_box_shape            = ("Added Box Shape",                     "Failed to add Box Shape")
    cylinder_above_terrain = ("Cylinder position above ground",   "Cylinder is not above the ground")
    time_out               = ("No time out occurred",             "A time out occurred, please validate level setup")
    touched_ground         = ("Touched ground before time out",   "Did not touch ground before time out")
    exit_game_mode         = ("Exited game mode",                 "Couldn't exit game mode")
# fmt: on


def ShapeCollider_CylinderShapeCollides():
    """
    Summary:
    Runs a test to make sure that a PhysX Rigid Body and Cylinder Shape Collider can successfully collide with a PhysX Terrain entity.

    Level Description:
    A cylinder with rigid body and collider is positioned above a PhysX terrain. 

    Expected Outcome:
    Once game mode is entered, the cylinder should fall toward and collide with the terrain.

    Steps:
    1) Open level and create terrain Entity.
    2) Enter Game Mode.
    3) Retrieve entities and positions
    4) Wait for cylinder to collide with terrain OR time out
    5) Exit game mode
    6) Close the editor

    :return:
    """
    import os
    import sys


    from editor_python_test_tools.editor_entity_utils import EditorEntity
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.math as math
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from consts.physics import PHYSX_SHAPE_COLLIDER

    # Global time out
    TIME_OUT = 1.0

    # 1) Open level 
    helper.init_idle()
    helper.open_level("Physics", "ShapeCollider_CylinderShapeCollides")
    
    # Create terrain entity 
    terrain = EditorEntity.create_editor_entity_at([30.0, 30.0, 33.96], "Terrain")
    terrain.add_component("PhysX Static Rigid Body")
    Report.result(Tests.create_terrain, terrain.id.IsValid())
    
    terrain.add_component(PHYSX_SHAPE_COLLIDER)
    Report.result(Tests.add_physx_shape_collider, terrain.has_component(PHYSX_SHAPE_COLLIDER))

    box_shape_component = terrain.add_component("Box Shape")
    Report.result(Tests.add_box_shape, terrain.has_component("Box Shape"))

    box_shape_component.set_component_property_value("Box Shape|Box Configuration|Dimensions", 
                                                     math.Vector3(1024.0, 1024.0, 0.01))
    
    # 2)Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve entities and positions
    cylinder_id = general.find_game_entity("PhysX_Cylinder")
    Report.critical_result(Tests.find_cylinder, cylinder_id.IsValid())
    
    
    terrain_id = general.find_game_entity("Terrain")
    Report.critical_result(Tests.find_terrain, terrain_id.IsValid())    


    cylinder_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", cylinder_id)
    #Cylinder position is 64,84,35
    terrain_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", terrain_id)
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

    # 4) Wait for the cylinder to hit the ground OR time out
    test_completed = helper.wait_for_condition((lambda: TouchGround.value), TIME_OUT)

    Report.critical_result(Tests.time_out, test_completed)
    Report.result(Tests.touched_ground, TouchGround.value)

    # 5) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ShapeCollider_CylinderShapeCollides)
