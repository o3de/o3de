"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C4044695
Test Case Title : Verify that when you add a multiple surface fbx in PxMesh in PhysxCollider,
                  multiple number of Material Slots populate in the Materials Section

"""


# fmt: off
class Tests():
    create_entity          = ("Created test entity",                         "Failed to create test entity")
    mesh_added             = ("Added Mesh component",                        "Failed to add Mesh component")
    physx_collider_added   = ("Added PhysX Collider component",              "Failed to add PhysX Collider component")
    shape_is_correct       = ("PhysX Collider Shape is correct",             "PhysX Collider Shape is not PhysicsAsset")
    assign_mesh_asset      = ("Assigned Mesh asset to Mesh component",       "Failed to assign mesh asset to Mesh component")
    assign_px_mesh_asset   = ("Assigned PxMesh asset to Collider component", "Failed to assign PxMesh asset to Collider component")
    count_mesh_surface     = ("Multiple slots show under materials",         "Failed to show required surface materials")
# fmt: on


def Collider_MultipleSurfaceSlots():
    """
    Summary:
     Create entity with Mesh and PhysX Collider components and assign a fbx file in both the components.
     Verify that the fbx is properly fitting the mesh.

    Expected Behavior:
     1) The fbx is properly fitting the mesh.
     2) Multiple material slots show up under Materials section in the PhysX Collider component and that
     they correspond to the number of surfaces as designed in the mesh.

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add Mesh and Physics collider components
     4) Select the PhysicsAsset shape in the PhysX Collider component
     5) Assign the fbx file in PhysX Mesh and Mesh component
     6) Check if multiple material slots show up under Materials section in the PhysX Collider component
     
    :return: None
    """
    # Builtins
    import os

    # Helper Files

    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.asset_utils import Asset

    # Constants
    PHYSICS_ASSET_INDEX = 7  # Hardcoded enum index value for Shape property
    SURFACE_TAG_COUNT = 4  # Number of surface tags included in used asset

    # Asset paths
    STATIC_MESH = os.path.join("assets", "Physics", "Collider_MultipleSurfaceSlots", "test.azmodel")
    PHYSX_MESH = os.path.join("assets", "Physics","Collider_MultipleSurfaceSlots", "test.pxmesh")

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("Physics", "Base")

    # 2) Create an entity
    test_entity = Entity.create_editor_entity("test_entity")
    Report.result(Tests.create_entity, test_entity.id.IsValid())

    # 3) Add Mesh and Physics collider components
    mesh_component = test_entity.add_component("Mesh")
    Report.result(Tests.mesh_added, test_entity.has_component("Mesh"))

    collider_component = test_entity.add_component("PhysX Collider")
    Report.result(Tests.physx_collider_added, test_entity.has_component("PhysX Collider"))

    # 4) Select the PhysicsAsset shape in the PhysX Collider component
    collider_component.set_component_property_value("Shape Configuration|Shape", PHYSICS_ASSET_INDEX)
    value_to_test = collider_component.get_component_property_value("Shape Configuration|Shape")
    Report.result(Tests.shape_is_correct, value_to_test == PHYSICS_ASSET_INDEX)

    # 5) Assign the fbx file in PhysX Mesh and Mesh component
    px_asset = Asset.find_asset_by_path(PHYSX_MESH)
    collider_component.set_component_property_value("Shape Configuration|Asset|PhysX Mesh", px_asset.id)
    px_asset.id = collider_component.get_component_property_value("Shape Configuration|Asset|PhysX Mesh")
    Report.result(Tests.assign_px_mesh_asset, px_asset.get_path().lower() == PHYSX_MESH.replace(os.sep, "/").lower())

    mesh_asset = Asset.find_asset_by_path(STATIC_MESH)
    mesh_component.set_component_property_value("Controller|Configuration|Mesh Asset", mesh_asset.id)
    mesh_asset.id = mesh_component.get_component_property_value("Controller|Configuration|Mesh Asset")
    Report.result(Tests.assign_mesh_asset, mesh_asset.get_path().lower() == STATIC_MESH.replace(os.sep, "/").lower())

    # 6) Check if multiple material slots show up under Materials section in the PhysX Collider component
    pte = collider_component.get_property_tree()
    def get_surface_count():
        count = pte.get_container_count("Collider Configuration|Physics Materials|Slots")
        return count.GetValue()

    Report.result(
        Tests.count_mesh_surface, helper.wait_for_condition(lambda: get_surface_count() == SURFACE_TAG_COUNT, 1.0)
    )


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_MultipleSurfaceSlots)
