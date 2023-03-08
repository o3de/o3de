"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PhysX_Primitive_Collider_Component_CRUD():
    """
    Summary:
    Creating a PhysX Primitive Collider and setting values to all properties that are supported via the Editor Context Bus.

    Expected Behavior:
    Validates component Created, Read, Update, Delete (CRUD) and performs expected operations.

    PhysX Primitive Collider Components:
     - PhysX Primitive Collider

    Test Steps:
     1) Add an Entity to manipulate
     2) Add a PhysX Primitive Collider Component to Entity
     3) Set Box Shape and Child Properties
     4) Set Capsule Shape and Child Properties
     5) Set Cylinder Shape and Child Properties
     6) Set Sphere Shape and Child Properties
     7) Set General Properties
     8) Delete Component

    :return: None
    """

    # Helper file Imports
    import os

    from editor_python_test_tools.editor_component.editor_physx_primitive_collider import EditorPhysxPrimitiveCollider as PhysxPrimitiveCollider
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Tracer, TestHelper
    from editor_python_test_tools.editor_component.editor_component_validation import (
        validate_property_switch_toggle, validate_vector3_property, validate_float_property,
        validate_integer_property)
    from editor_python_test_tools.editor_component.test_values.common_test_values import (
        VECTOR3_TESTS_NEGATIVE_EXPECT_PASS, FLOAT_HEIGHT_TESTS)
    from editor_python_test_tools.editor_component.test_values.phsyx_collider_test_values import (
        COLLIDER_RADIUS_TESTS, CYLINDER_HEIGHT_TESTS, CONTACT_OFFSET_TESTS,
        REST_OFFSET_TESTS, CYLINDER_SUBDIVISION_TESTS)

    from consts.general import Strings
    from consts.physics import PHYSX_PRIMITIVE_COLLIDER

    # 0) Pre-conditions
    physx_material = os.path.join("physx", "glass.physxmaterial")

    TestHelper.init_idle()
    TestHelper.open_level("", "Base")

    with Tracer() as section_tracer:
    # 1 ) Add an Entity to manipulate
        phsyx_collider_entity = EditorEntity.create_editor_entity("TestEntity")

    # 2) Add a PhysX Primitive Collider Component to Entity
        physx_collider = PhysxPrimitiveCollider(phsyx_collider_entity)

    # 3) Set Box Shape and Child Properties
        physx_collider.set_box_shape()

        validate_vector3_property("Box Dimensions", physx_collider.get_box_dimensions, physx_collider.set_box_dimensions, PHYSX_PRIMITIVE_COLLIDER, VECTOR3_TESTS_NEGATIVE_EXPECT_PASS)

    # 4) Set Capsule Shape and Child Properties
        physx_collider.set_capsule_shape()

        validate_float_property("Capsule Height", physx_collider.get_capsule_height, physx_collider.set_capsule_height, PHYSX_PRIMITIVE_COLLIDER, FLOAT_HEIGHT_TESTS)
        validate_float_property("Capsule Radius", physx_collider.get_capsule_radius, physx_collider.set_capsule_radius, physx_collider.component.get_component_name(), COLLIDER_RADIUS_TESTS)

    # 5) Set Cylinder Shape and Child Properties
        physx_collider.set_cylinder_shape()

        validate_float_property("Cylinder Height", physx_collider.get_cylinder_height, physx_collider.set_cylinder_height, PHYSX_PRIMITIVE_COLLIDER,  CYLINDER_HEIGHT_TESTS)
        validate_float_property("Cylinder Radius", physx_collider.get_cylinder_radius, physx_collider.set_cylinder_radius, PHYSX_PRIMITIVE_COLLIDER, COLLIDER_RADIUS_TESTS)

        validate_integer_property("Cylinder Subdivision", physx_collider.get_cylinder_subdivision, physx_collider.set_cylinder_subdivision, PHYSX_PRIMITIVE_COLLIDER, CYLINDER_SUBDIVISION_TESTS)

    # 6) Set Sphere Shape and Child Properties
        physx_collider.set_sphere_shape()

        validate_float_property("Sphere Radius", physx_collider.get_sphere_radius, physx_collider.set_sphere_radius, PHYSX_PRIMITIVE_COLLIDER, COLLIDER_RADIUS_TESTS)

    # 7) Set General Properties
        validate_property_switch_toggle("Is Trigger", physx_collider.get_is_trigger, physx_collider.set_is_trigger, PHYSX_PRIMITIVE_COLLIDER)
        validate_property_switch_toggle("Is Simulated", physx_collider.get_is_simulated, physx_collider.set_is_simulated, PHYSX_PRIMITIVE_COLLIDER)
        validate_property_switch_toggle("In Scene Queries", physx_collider.get_in_scene_queries, physx_collider.set_in_scene_queries, PHYSX_PRIMITIVE_COLLIDER)

        validate_vector3_property("Offset", physx_collider.get_offset, physx_collider.set_offset, PHYSX_PRIMITIVE_COLLIDER, VECTOR3_TESTS_NEGATIVE_EXPECT_PASS)

        # o3de/o3de#13223 - Add Quaternion Property Validator to validate Rotation property
        # o3de/o3de#12634 - Get PhysX Primitive Collider Tag to set properly.

        validate_float_property("Contact Offset", physx_collider.get_contact_offset, physx_collider.set_contact_offset, PHYSX_PRIMITIVE_COLLIDER, CONTACT_OFFSET_TESTS)
        validate_float_property("Rest Offset", physx_collider.get_rest_offset, physx_collider.set_rest_offset, PHYSX_PRIMITIVE_COLLIDER, REST_OFFSET_TESTS)

        validate_property_switch_toggle("Draw Collider", physx_collider.get_draw_collider, physx_collider.set_draw_collider, PHYSX_PRIMITIVE_COLLIDER)

        # o3de/o3de#12503 PhysX Primitive Collider Component's Physic Material field(s) return unintuitive property tree paths.

    # 8) Delete Component
        physx_collider.component.remove()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PhysX_Primitive_Collider_Component_CRUD)

