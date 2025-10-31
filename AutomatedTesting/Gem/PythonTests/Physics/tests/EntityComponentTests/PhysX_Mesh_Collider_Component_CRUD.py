"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PhysX_Mesh_Collider_Component_CRUD():
    """
    Summary:
    Creating a PhysX Mesh Collider and setting values to all properties that are supported via the Editor Context Bus.

    Expected Behavior:
    Validates component Created, Read, Update, Delete (CRUD) and performs expected operations.

    PhysX Mesh Collider Components:
     - PhysX Mesh Collider

    Test Steps:
     1) Add an Entity to manipulate
     2) Add a PhysX Mesh Collider Component to Entity
     3) Set General Properties
     4) Set PhyicsAsset Shape and Child Properties
     5) Delete Component

    :return: None
    """

    # Helper file Imports
    import os

    from editor_python_test_tools.editor_component.editor_physx_mesh_collider import EditorPhysxMeshCollider as PhysxMeshCollider
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Tracer, TestHelper
    from editor_python_test_tools.editor_component.editor_component_validation import (
        validate_property_switch_toggle, validate_vector3_property, validate_float_property,
        validate_asset_property)
    from editor_python_test_tools.editor_component.test_values.common_test_values import (
        VECTOR3_TESTS_NEGATIVE_EXPECT_PASS)
    from editor_python_test_tools.editor_component.test_values.phsyx_collider_test_values import (
        CONTACT_OFFSET_TESTS, REST_OFFSET_TESTS)

    from consts.general import Strings
    from consts.physics import PHYSX_MESH_COLLIDER

    # 0) Pre-conditions
    physx_mesh = os.path.join("objects", "_primitives", "_box_1x1.fbx.pxmesh")
    physx_material = os.path.join("physx", "glass.physxmaterial")

    TestHelper.init_idle()
    TestHelper.open_level("", "Base")

    with Tracer() as section_tracer:
    # 1 ) Add an Entity to manipulate
        phsyx_collider_entity = EditorEntity.create_editor_entity("TestEntity")

    # 2) Add a PhysX Collider Component to Entity
        physx_collider = PhysxMeshCollider(phsyx_collider_entity)

    # 3) Set General Properties
        validate_property_switch_toggle("Is Trigger", physx_collider.get_is_trigger, physx_collider.set_is_trigger, PHYSX_MESH_COLLIDER)
        validate_property_switch_toggle("Is Simulated", physx_collider.get_is_simulated, physx_collider.set_is_simulated, PHYSX_MESH_COLLIDER)
        validate_property_switch_toggle("In Scene Queries", physx_collider.get_in_scene_queries, physx_collider.set_in_scene_queries, PHYSX_MESH_COLLIDER)

        validate_vector3_property("Offset", physx_collider.get_offset, physx_collider.set_offset, PHYSX_MESH_COLLIDER, VECTOR3_TESTS_NEGATIVE_EXPECT_PASS)

        # o3de/o3de#13223 - Add Quaternion Property Validator to validate Rotation property
        # o3de/o3de#12634 - Get PhysX Mesh Collider Tag to set properly.

        validate_float_property("Contact Offset", physx_collider.get_contact_offset, physx_collider.set_contact_offset, PHYSX_MESH_COLLIDER, CONTACT_OFFSET_TESTS)
        validate_float_property("Rest Offset", physx_collider.get_rest_offset, physx_collider.set_rest_offset, PHYSX_MESH_COLLIDER, REST_OFFSET_TESTS)

        validate_property_switch_toggle("Draw Collider", physx_collider.get_draw_collider, physx_collider.set_draw_collider, PHYSX_MESH_COLLIDER)

        # o3de/o3de#12503 PhysX Mesh Collider Component's Physic Material field(s) return unintuitive property tree paths.

    # 4) Set PhyicsAsset Shape Properties
        validate_asset_property("PhysX Mesh Asset", physx_collider.get_physx_mesh, physx_collider.set_physx_mesh, PHYSX_MESH_COLLIDER, physx_mesh)
        validate_vector3_property("PhysX Mesh Asset Scale", physx_collider.get_physx_mesh_asset_scale, physx_collider.set_physx_mesh_asset_scale, PHYSX_MESH_COLLIDER, VECTOR3_TESTS_NEGATIVE_EXPECT_PASS)
        validate_property_switch_toggle("Use Physics Materials From Asset", physx_collider.get_use_physics_materials_from_asset, physx_collider.set_use_physics_materials_from_asset, PHYSX_MESH_COLLIDER)

    # 5) Delete Component
        physx_collider.component.remove()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PhysX_Mesh_Collider_Component_CRUD)

