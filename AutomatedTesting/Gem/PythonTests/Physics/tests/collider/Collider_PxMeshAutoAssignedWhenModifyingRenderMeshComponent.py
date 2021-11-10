"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C14861502
Test Case Title : Verify PxMesh is auto-assigned in collider when Mesh is assigned in Rendering Mesh component

"""


# fmt: off
class Tests():
    create_entity          = ("Created test entity",             "Failed to create test entity")
    mesh_added             = ("Added Mesh component",            "Failed to add Mesh component")
    physx_collider_added   = ("Added PhysX Collider component",  "Failed to add PhysX Collider component")
    shape_default_at_start = ("Shape was correct initially",     "Default shape was not correct")
    automatic_shape_change = ("Shape was changed automatically", "Shape failed to change automatically")
# fmt: on


def Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent():
    """
    Summary:
     Create entity with Mesh and PhysX Collider components, then assign a render mesh to the Mesh component

    Expected Behavior:
     The physics asset in PhysX Collider component is auto-assigned after adding the render mesh

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add Mesh and PhysX Collider component
     4) Verify no physics asset is auto-assigned.
     5) Assign a render mesh asset to Mesh component (the fbx mesh having both Static mesh and PhysX collision Mesh)
     6) The physics asset in PhysX Collider component is auto-assigned.

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
    from editor_python_test_tools.asset_utils import Asset

    # Open 3D Engine Imports
    import azlmbr.legacy.general as general

    MESH_ASSET_PATH = os.path.join("Objects", "SphereBot", "r0-b_body.azmodel")
    MESH_PROPERTY_PATH = "Controller|Configuration|Mesh Asset"
    TESTED_PROPERTY_PATH = "Shape Configuration|Asset|PhysX Mesh"

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("Physics", "Base")

    # 2) Create an entity
    test_entity = Entity.create_editor_entity("test_entity")
    Report.result(Tests.create_entity, test_entity.id.IsValid())

    # 3) Add Mesh and PhysX Collider component
    mesh_component = test_entity.add_component("Mesh")
    Report.result(Tests.mesh_added, test_entity.has_component("Mesh"))

    test_component = test_entity.add_component("PhysX Collider")
    Report.result(Tests.physx_collider_added, test_entity.has_component("PhysX Collider"))

    # 4) Verify no physics asset is auto-assigned
    value_to_test = test_component.get_component_property_value(TESTED_PROPERTY_PATH)
    Report.result(Tests.shape_default_at_start, value_to_test == azlmbr.asset.AssetId())

    # 5) Assign a render mesh asset to Mesh component (the fbx mesh having both Static mesh and PhysX collision Mesh)
    asset_value = Asset.find_asset_by_path(MESH_ASSET_PATH)
    mesh_component.set_component_property_value(MESH_PROPERTY_PATH, asset_value.id)

    # 6) The physics asset in PhysX Collider component is auto-assigned.
    general.idle_wait(1.0)  # Gives the script a moment for the value to update before grabbing it
    value_to_test = test_component.get_component_property_value(TESTED_PROPERTY_PATH)
    asset = Asset(value_to_test)
    Report.result(Tests.automatic_shape_change, "r0-b_body" in asset.get_path())


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_PxMeshAutoAssignedWhenModifyingRenderMeshComponent)
