"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PhysX_Collider_Component_CRUD():
    """
    Summary:
    Creating a PhysX Collider and setting values to all properties that are supported via the Editor Context Bus.

    Expected Behavior:
    Validates component Created, Read, Update, Delete (CRUD) and performs expected operations.

    PhysX Collider Components:
     - PhysX Collider

    Test Steps:
     1) Add an Entity to manipulate
     2) Add a PhysX Collider Component to Entity
     3) Set Box Shape and Child Properties
     4) Set Capsule Shape and Child Properties
     5) Set Cylinder Shape and Child Properties
     6) Set Sphere Shape and Child Properties
     7) Set General Properties
     8) Set PhyicsAsset Shape and Child Properties
     9) Delete Component

    :return: None
    """

    # Helper file Imports
    import os

    from editor_python_test_tools.editor_component.editor_physx_collider import EditorPhysxCollider as PhysxCollider
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Tracer, TestHelper
    from editor_python_test_tools.editor_component.editor_component_validation import validate_property_switch_toggle

    from consts.general import Strings

    # 0) Pre-conditions
    physx_mesh = os.path.join("objects", "_primitives", "_box_1x1.pxmesh")
    physx_material = os.path.join("physx", "glass.physxmaterial")
    component_name = "PhysX Collider"

    TestHelper.init_idle()
    TestHelper.open_level("", "Base")

    with Tracer() as section_tracer:
    # 1 ) Add an Entity to manipulate
        phsyx_collider_entity = EditorEntity.create_editor_entity("TestEntity")

    # 2) Add a PhysX Collider Component to Entity
        physx_collider = PhysxCollider(phsyx_collider_entity)

    # 3) Set Box Shape and Child Properties
        physx_collider.set_box_shape()

        physx_collider.set_box_dimensions(256.0, 256.0, 256.0)
        physx_collider.set_box_dimensions(2.5, 2.5, 2.5)
        physx_collider.set_box_dimensions(0.0, 0.0, 0.0)
        physx_collider.set_box_dimensions(-1.0, -10.0, -256.0)

    # 4) Set Capsule Shape and Child Properties
        physx_collider.set_capsule_shape()

        physx_collider.set_capsule_height(0.0)
        physx_collider.set_capsule_height(-1.0)
        physx_collider.set_capsule_height(2.5)
        physx_collider.set_capsule_height(256.0)

        #physx_collider.set_capsule_radius(0)
        physx_collider.set_capsule_radius(-1.0)
        physx_collider.set_capsule_radius(2.5)
        physx_collider.set_capsule_radius(256.0)

    # 5) Set Cylinder Shape and Child Properties
        physx_collider.set_cylinder_shape()

        physx_collider.set_cylinder_height(0.0)
        physx_collider.set_cylinder_height(-1.0)
        physx_collider.set_cylinder_height(2.5)
        physx_collider.set_cylinder_height(256.0)

        physx_collider.set_cylinder_radius(0.0)
        physx_collider.set_cylinder_radius(-1.0)
        physx_collider.set_cylinder_radius(2.5)
        physx_collider.set_cylinder_radius(256.0)

        #physx_collider.set_cylinder_subdivision(0)  # o3de/o3de#12608 - Crash if subdivision set below 3
        # physx_collider.set_cylinder_subdivision(-1)  # o3de/o3de#12608 - Crash if subdivision set below 3
        physx_collider.set_cylinder_subdivision(3)
        physx_collider.set_cylinder_subdivision(125)
        # physx_collider.set_cylinder_subdivision(256)  # GHI #12608 - Crash if subdivision set above 125

    # 6) Set Sphere Shape and Child Properties
        physx_collider.set_sphere_shape()

        physx_collider.set_sphere_radius(0.0)
        physx_collider.set_sphere_radius(-1.0)
        physx_collider.set_sphere_radius(2.5)
        physx_collider.set_sphere_radius(256.0)

        # 7) Set General Properties
        validate_property_switch_toggle(physx_collider.get_is_trigger, physx_collider.set_is_trigger, component_name, "Is Trigger")
        validate_property_switch_toggle(physx_collider.get_is_trigger, physx_collider.set_is_trigger, component_name, "Is Trigger")

        validate_property_switch_toggle(physx_collider.get_is_simulated, physx_collider.set_is_simulated, component_name, "Is Simulated")
        validate_property_switch_toggle(physx_collider.get_is_simulated, physx_collider.set_is_simulated, component_name, "Is Simulated")

        validate_property_switch_toggle(physx_collider.get_in_scene_queries, physx_collider.set_in_scene_queries, component_name, "In Scene Queries")
        validate_property_switch_toggle(physx_collider.get_in_scene_queries, physx_collider.set_in_scene_queries, component_name, "In Scene Queries")

        physx_collider.set_offset(0.0, 0.0, 0.0)
        physx_collider.set_offset(-1.0, -1.0, -1.0)
        physx_collider.set_offset(2.5, 2.5, 2.5)
        physx_collider.set_offset(256.0, 256.0, 256.0)

        physx_collider.set_rotation(0.0, 0.0, 0.0)
        physx_collider.set_rotation(-1.0, -1.0, -1.0)
        physx_collider.set_rotation(2.5, 2.5, 2.5)
        physx_collider.set_rotation(255.0, 255.0, 255.0)

        # o3de/o3de#12634 - For some reason I can't get this to work. Says it takes an AZStd::string,
        #  but it won't take a python string or a character.
        # physx_collider.set_tag(Strings.NUMBER)
        # physx_collider.set_tag(Strings.CHARACTER)
        # physx_collider.set_tag(Strings.ESCAPED_SPACE)
        # physx_collider.set_tag(Strings.EMPTY_STRING)
        # physx_collider.set_tag(Strings.ONLY_SPACE)physx_collider.set_tag(Strings.NUMBER)

        physx_collider.set_rest_offset(0.0)
        physx_collider.set_rest_offset(-1.0)
        physx_collider.set_rest_offset(2.5)
        physx_collider.set_rest_offset(255.0)

        physx_collider.set_contact_offset(0.0)
        physx_collider.set_contact_offset(-1.0)
        physx_collider.set_contact_offset(2.5)
        physx_collider.set_contact_offset(255.0)

        validate_property_switch_toggle(physx_collider.get_draw_collider, physx_collider.set_draw_collider, component_name, "Draw Collider")
        validate_property_switch_toggle(physx_collider.get_is_trigger, physx_collider.set_is_trigger, component_name, "Draw Collider")

        # o3de/o3de#12503 PhysX Collider Component's Physic Material field(s) return unintuitive property tree paths.
        # physx_collider.set_physx_material_from_path(physx_material)

    # 8) Set PhyicsAsset Shape and Child Properties
        physx_collider.set_physicsasset_shape()
        physx_collider.set_physx_mesh_from_path(physx_mesh)

        physx_collider.set_physx_mesh_asset_scale(0.0, 0.0, 0.0)
        physx_collider.set_physx_mesh_asset_scale(-1.0, -1.0, -1.0)
        physx_collider.set_physx_mesh_asset_scale(2.5, 2.5, 2.5)
        physx_collider.set_physx_mesh_asset_scale(255.0, 255.0, 255.0)

        validate_property_switch_toggle(physx_collider.get_use_physics_materials_from_asset, physx_collider.set_use_physics_materials_from_asset, component_name, "Use Physics Materials From Asset")
        validate_property_switch_toggle(physx_collider.get_use_physics_materials_from_asset, physx_collider.set_use_physics_materials_from_asset, component_name, "Use Physics Materials From Asset")

    # 9) Delete Component
        physx_collider.component.remove()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PhysX_Collider_Component_CRUD)

