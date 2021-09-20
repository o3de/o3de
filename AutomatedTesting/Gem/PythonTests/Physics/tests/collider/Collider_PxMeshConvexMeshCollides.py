"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C4982803
Test Case Title : Verify that when the shape Physics Asset is selected,
    PxMesh option gets enabled and a Px Mesh can be selected and assigned to the object

"""

# fmt: off
class Tests():
    create_collider_entity   = ("Entity created successfully",         "Failed to create Entity")
    mesh_added               = ("Added Mesh component",                "Failed to add Mesh component")
    physx_collider_added     = ("Added PhysX Collider component",      "Failed to add PhysX Collider component")
    physx_rigid_body_added   = ("Added PhysX Rigid Body component",    "Failed to add PhysX Rigid Body component")
    add_physics_asset_shape  = ("Added shape as physics asset",        "Failed to add shape ")
    assign_fbx_mesh          = ("Assigned fbx mesh",                   "Failed to assign fbx mesh") 
    create_terrain           = ("Terrain entity created successfully", "Failed to create Terrain Entity")
    add_physx_shape_collider = ("Added PhysX Shape Collider",          "Failed to add PhysX Shape Collider")
    add_box_shape            = ("Added Box Shape",                     "Failed to add Box Shape")
    enter_game_mode          = ("Entered game mode",                   "Failed to enter game mode")
    exit_game_mode           = ("Exited game mode",                    "Failed to exit game mode")
    test_collision           = ("Entity collided with terrain",        "Failed to collide with terrain")
# fmt: on


def Collider_PxMeshConvexMeshCollides():
    """
    Summary:
    Load level with Entity above ground having PhysX Collider, PhysX Rigid Body and Mesh components.
    Verify that the entity falls on the ground and collides with the terrain when fbx convex mesh is added to collider.

    Expected Behavior:
    The entity falls on the ground and collides with the terrain.

    Test Steps:
     1) Load the level
     2) Create test entity above the ground
     3) Add PhysX Collider, PhysX Rigid Body and Mesh components.
     4) Add the Shape as PhysicsAsset and select fbx convex mesh in PhysX Collider component.
     5) Create terrain entity with physx terrain
     6) Enter game mode
     7) Verify that the entity falls on the ground and collides with the terrain.

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Builtins
    import os

    # Helper Files

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.asset_utils import Asset
    import azlmbr.math as math

    # Open 3D Engine Imports
    import azlmbr
    import azlmbr.legacy.general as general

    # Constants
    PHYSICS_ASSET_INDEX = 7  # Hardcoded enum index value for Shape property
    MESH_ASSET_PATH = os.path.join("assets", "Physics", "Collider_PxMeshConvexMeshCollides", "spherebot", "r0-b_body.pxmesh")
    TIMEOUT = 2.0

    helper.init_idle()
    # 1) Load the level
    helper.open_level("Physics", "Base")

    # 2) Create test entity
    collider = EditorEntity.create_editor_entity_at([512.0, 512.0, 33.0], "Collider")
    Report.result(Tests.create_collider_entity, collider.id.IsValid())

    # 3) Add PhysX Collider, PhysX Rigid Body and Mesh components.
    collider_component = collider.add_component("PhysX Collider")
    Report.result(Tests.physx_collider_added, collider.has_component("PhysX Collider"))

    collider.add_component("PhysX Rigid Body")
    Report.result(Tests.physx_rigid_body_added, collider.has_component("PhysX Rigid Body"))

    collider.add_component("Mesh")
    Report.result(Tests.mesh_added, collider.has_component("Mesh"))

    # 4) Add the Shape as PhysicsAsset and select fbx convex mesh in PhysX Collider component.
    collider_component.set_component_property_value("Shape Configuration|Shape", PHYSICS_ASSET_INDEX)
    value_to_test = collider_component.get_component_property_value("Shape Configuration|Shape")
    Report.result(Tests.add_physics_asset_shape, value_to_test == PHYSICS_ASSET_INDEX)

    mesh_asset = Asset.find_asset_by_path(MESH_ASSET_PATH)
    collider_component.set_component_property_value("Shape Configuration|Asset|PhysX Mesh", mesh_asset.id)
    mesh_asset.id = collider_component.get_component_property_value("Shape Configuration|Asset|PhysX Mesh")
    Report.result(Tests.assign_fbx_mesh, mesh_asset.get_path().lower() == MESH_ASSET_PATH.replace(os.sep, "/").lower())

    # 5) Create terrain entity with physx terrain
    terrain = EditorEntity.create_editor_entity_at([512.0, 512.0, 31.0], "Terrain")
    Report.result(Tests.create_terrain, terrain.id.IsValid())

    terrain.add_component("PhysX Shape Collider")
    Report.result(Tests.add_physx_shape_collider, terrain.has_component("PhysX Shape Collider"))

    box_shape_component = terrain.add_component("Box Shape")
    Report.result(Tests.add_box_shape, terrain.has_component("Box Shape"))

    box_shape_component.set_component_property_value("Box Shape|Box Configuration|Dimensions", 
                                                     math.Vector3(1024.0, 1024.0, 1.0))
    # 6) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 7) Verify that the entity falls on the ground and collides with the terrain
    class Collider:
        id = general.find_game_entity("Collider")
        touched_ground = False

    terrain_id = general.find_game_entity("Terrain")
    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(terrain_id):
            Report.info("Touched ground")
            Collider.touched_ground = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(Collider.id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)

    helper.wait_for_condition(lambda: Collider.touched_ground, TIMEOUT)
    Report.result(Tests.test_collision, Collider.touched_ground)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_PxMeshConvexMeshCollides)
