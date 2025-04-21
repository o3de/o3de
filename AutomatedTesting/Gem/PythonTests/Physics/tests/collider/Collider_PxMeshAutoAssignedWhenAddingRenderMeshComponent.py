"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C14861501
Test Case Title : Verify PxMesh is auto-assigned when Collider component is added after Rendering Mesh component

"""


# fmt: off
class Tests():
    create_entity          = ("Created test entity",                   "Failed to create test entity")
    mesh_added             = ("Added Mesh component",                  "Failed to add Mesh component")
    physx_collider_added   = ("Added PhysX Mesh Collider component",   "Failed to add PhysX Mesh Collider component")
    assign_model_asset     = ("Assigned Model asset to Mesh component", "Failed to assign model asset to Mesh component")
    automatic_shape_change = ("Shape was changed automatically",       "Shape failed to change automatically")
# fmt: on


def Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent():
    """
    Summary:
     Create entity with Mesh component and assign a render mesh to the Mesh component. Add Physics Mesh Collider component
     and Verify that the physics mesh asset is auto-assigned.

    Expected Behavior:
     The physics asset in PhysX Mesh Collider component is auto-assigned

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add Mesh component
     4) Assign a render model asset to Mesh component (the fbx mesh having both Static mesh and PhysX collision Mesh)
     5) Add PhysX Mesh Collider component
     6) The physics asset in PhysX Mesh Collider component is auto-assigned.

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
    from consts.physics import PHYSX_MESH_COLLIDER

    import editor_python_test_tools.hydra_editor_utils as hydra

    # Asset paths
    STATIC_MESH = os.path.join("assets", "Physics", "Collider_PxMeshAutoAssigned", "spherebot", "r0-b_body.fbx.azmodel")
    PHYSX_MESH = os.path.join(
        "assets", "Physics", "Collider_PxMeshAutoAssigned", "spherebot", "r0-b_body.fbx.pxmesh"
    )

    # 1) Load the empty level
    hydra.open_base_level()

    # 2) Create an entity
    test_entity = Entity.create_editor_entity("test_entity")
    test_entity.add_component("PhysX Static Rigid Body")
    Report.result(Tests.create_entity, test_entity.id.IsValid())

    # 3) Add Mesh component
    mesh_component = test_entity.add_component("Mesh")
    Report.result(Tests.mesh_added, test_entity.has_component("Mesh"))

    # 4) Assign a render model asset to Mesh component (the fbx mesh having both Static mesh and PhysX collision Mesh)
    model_asset = Asset.find_asset_by_path(STATIC_MESH)
    mesh_component.set_component_property_value("Controller|Configuration|Model Asset", model_asset.id)
    model_asset.id = mesh_component.get_component_property_value("Controller|Configuration|Model Asset")
    Report.result(Tests.assign_model_asset, model_asset.get_path().lower() == STATIC_MESH.replace(os.sep, "/").lower())

    # 5) Add PhysX Mesh Collider component
    test_component = test_entity.add_component(PHYSX_MESH_COLLIDER)
    Report.result(Tests.physx_collider_added, test_entity.has_component(PHYSX_MESH_COLLIDER))

    # 6) The physics asset in PhysX Mesh Collider component is auto-assigned.
    asset_id = test_component.get_component_property_value("Shape Configuration|Asset|PhysX Mesh")
    test_asset = Asset(asset_id)
    Report.result(Tests.automatic_shape_change, test_asset.get_path().lower() == PHYSX_MESH.replace(os.sep, "/").lower())


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_PxMeshAutoAssignedWhenAddingRenderMeshComponent)
