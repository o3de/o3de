"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Component_TestComponentInit():

    """
    Summary:
    Tests an EditoryEntityComponent class to verify the class and its property classes initialize properly. This is
    a POC test for verifying the preliminary work on the editor entity component test framework.

    Expected Behavior:
    An entity is added to the level and given a Physx Dynamic Rigid Body Component. Modifications to the component's
    default values are reflected in the EditorEntityComponent

    Test Steps:
    1) Open level
    2) Create entity
    3) Add a physx component to the entity.
    4) Modify one of each property types on the component (Integer, float, Vector3 etc)
    5) Get the property values and verify they match what was set in the previous step

    :return: None
    """
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    from editor_python_test_tools.utils import TestHelper
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_component.PhysXDynamicRigidBodyComponent import PhysXDynamicRigidBodyComponent
    from editor_python_test_tools.editor_component.editor_component_validation import \
        (validate_vector3_property, validate_float_property, validate_property_switch_toggle, validate_integer_property)
    from editor_python_test_tools.editor_component.test_values.common_test_values import (
        INT_TESTS_NEGATIVE_EXPECT_FAIL, VECTOR3_TESTS_NEGATIVE_EXPECT_PASS, FLOAT_TESTS_NEGATIVE_EXPECT_PASS)

    general.idle_enable(True)

    TEST_ENTITY_NAME = "Test_Entity"
    TEST_VECTOR3_X = 101.1
    TEST_VECTOR3_Y = 202.2
    TEST_VECTOR3_Z = 303.3
    TEST_FLOAT = 10.5
    TEST_INT = 2
    TEST_BOOL = True

    # 1) Open a base level
    TestHelper.open_level("", "Base")

    # 2) Create entity
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity = EditorEntity.create_editor_entity_at(position, TEST_ENTITY_NAME)

    # 3) Add a dynamic rigid body component to the entity. keep a handle on the EditorEntityComponent object
    dynamic_rigid_body_component = PhysXDynamicRigidBodyComponent(editor_entity)

    # 4) Modify one of each property types on the component
    dynamic_rigid_body_component.initial_linear_velocity.set(TEST_VECTOR3_X, TEST_VECTOR3_Y,TEST_VECTOR3_Z)
    validate_vector3_property("Initial linear velocity",
                              dynamic_rigid_body_component.initial_linear_velocity.get,
                              dynamic_rigid_body_component.initial_linear_velocity.set, "PhysX Dynamic Rigid Body",
                              VECTOR3_TESTS_NEGATIVE_EXPECT_PASS)

    dynamic_rigid_body_component.linear_dampening.set(TEST_FLOAT)
    validate_float_property("Linear dampening",
                              dynamic_rigid_body_component.linear_dampening.get,
                              dynamic_rigid_body_component.linear_dampening.set, "PhysX Dynamic Rigid Body",
                              FLOAT_TESTS_NEGATIVE_EXPECT_PASS)

    dynamic_rigid_body_component.start_asleep.set(TEST_BOOL)
    validate_property_switch_toggle("Start asleep",
                                    dynamic_rigid_body_component.start_asleep.get,
                                    dynamic_rigid_body_component.start_asleep.set, "PhysX Dynamic Rigid Body")

    dynamic_rigid_body_component.solver_position_iterations.set(TEST_INT)
    validate_integer_property("Solver Position Iterations",
                              dynamic_rigid_body_component.solver_position_iterations.get,
                              dynamic_rigid_body_component.solver_position_iterations.set, "PhysX Dynamic Rigid Body",
                              INT_TESTS_NEGATIVE_EXPECT_FAIL)

    # 5) Get the property values and verify they match what was set in the previous step
    start_asleep = dynamic_rigid_body_component.start_asleep.get()
    position_iterations = dynamic_rigid_body_component.solver_position_iterations.get()

    assert start_asleep == TEST_BOOL, "Boolean was not set properly"
    assert position_iterations == TEST_INT, "Integer was not set properly"


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Component_TestComponentInit)
