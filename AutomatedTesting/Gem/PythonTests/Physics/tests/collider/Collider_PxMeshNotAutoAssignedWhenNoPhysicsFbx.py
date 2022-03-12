"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C14861504
Test Case Title : Verify if Rendering Mesh does not have a PhysX Collision Mesh fbx, then PxMesh is not auto-assigned

"""


# fmt: off
class Tests():
    create_entity        = ("Created test entity",                   "Failed to create test entity")
    mesh_added           = ("Added Mesh component",                  "Failed to add Mesh component")
    physx_collider_added = ("Added PhysX Collider component",        "Failed to add PhysX Collider component")
    assign_mesh_asset    = ("Assigned Mesh asset to Mesh component", "Failed to assign mesh asset to Mesh component")
    shape_not_assigned   = ("Shape is not auto assigned",            "Shape auto assigned unexpectedly")
    enter_game_mode      = ("Entered game mode",                     "Failed to enter game mode")
    warnings_found       = ("Warnings found in logs",                "No warnings found in logs")
# fmt: on


def Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx():
    """
    Summary:
     Create entity with Mesh component and assign a render mesh that has no physics asset to the Mesh component.
     Add Physics Collider component and Verify that the physics mesh asset is not auto-assigned.

    Expected Behavior:
     Following warning is logged in Game mode:
     "(PhysX) - EditorColliderComponent::BuildGameEntity. No asset assigned to Collider Component. Entity: <Entity Name>"

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add Mesh component
     4) Assign a render mesh asset to Mesh component (the fbx mesh having only Static mesh and no PxMesh)
     5) Add PhysX Collider component
     6) The physics asset in PhysX Collider component is not auto-assigned.
     7) Enter GameMode and check for warnings

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Builtins
    import os

    # Helper Files

    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    from editor_python_test_tools.asset_utils import Asset

    # Open 3D Engine Imports
    import azlmbr.asset as azasset

    # Asset paths
    STATIC_MESH = os.path.join("assets", "Physics", "Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx", "test_asset.azmodel")

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("Physics", "Base")

    # 2) Create an entity
    test_entity = Entity.create_editor_entity("test_entity")
    Report.result(Tests.create_entity, test_entity.id.IsValid())

    # 3) Add Mesh component
    mesh_component = test_entity.add_component("Mesh")
    Report.result(Tests.mesh_added, test_entity.has_component("Mesh"))

    # 4) Assign a render mesh asset to Mesh component (the fbx mesh having both Static mesh and PhysX collision Mesh)
    mesh_asset = Asset.find_asset_by_path(STATIC_MESH)
    mesh_component.set_component_property_value("Controller|Configuration|Mesh Asset", mesh_asset.id)
    mesh_asset.id = mesh_component.get_component_property_value("Controller|Configuration|Mesh Asset")
    Report.result(Tests.assign_mesh_asset, mesh_asset.get_path().lower() == STATIC_MESH.replace(os.sep, "/").lower())

    # 5) Add PhysX Collider component
    test_component = test_entity.add_component("PhysX Collider")
    Report.result(Tests.physx_collider_added, test_entity.has_component("PhysX Collider"))

    # 6) The physics asset in PhysX Collider component is not auto-assigned.
    asset_id = test_component.get_component_property_value("Shape Configuration|Asset|PhysX Mesh")
    # Comparing asset_id with Null/Invalid asset azlmbr.asset.AssetId() to check that asset is not auto assigned
    Report.result(Tests.shape_not_assigned, asset_id == azasset.AssetId())

    # 7) Enter GameMode and check for warnings
    with Tracer() as section_tracer:
        helper.enter_game_mode(Tests.enter_game_mode)
    # Checking if warning exist and the exact warning is caught in the expected lines in Test file
    Report.result(Tests.warnings_found, section_tracer.has_warnings)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_PxMeshNotAutoAssignedWhenNoPhysicsFbx)
