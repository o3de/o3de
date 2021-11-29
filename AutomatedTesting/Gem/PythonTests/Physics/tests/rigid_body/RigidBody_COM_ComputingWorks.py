"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976209
# Test Case Title : Verify that when Compute COM is enabled, the PhysX system computes the COM of the object on its own



# fmt: off
class Tests():
    enter_game_mode                     = ("Entered game mode",                                      "Failed to enter game mode")

    find_enabled_sphere                 = ("Enabled Sphere Found",                                   "Enabled Sphere not found")
    find_enabled_capsule                = ("Enabled Capsule Found",                                  "Enabled Capsule not found")
    find_enabled_box                    = ("Enabled Box Found",                                      "Enabled Box not found")
    find_enabled_physics_asset          = ("Enabled Physics Asset Found",                            "Enabled Physics Asset not found")

    find_disabled_sphere                = ("Disabled Sphere Found",                                  "Disabled Sphere not found")
    find_disabled_capsule               = ("Disabled Capsule Found",                                 "Disabled Capsule not found")
    find_disabled_box                   = ("Disabled Box Found",                                     "Disabled Box not found")
    find_disabled_physics_asset         = ("Disabled Physics Asset Found",                           "Disabled Physics Asset not found")

    enabled_sphere_COM_expected         = ("Enabled Sphere COM was close to expected value",         "Enabled Sphere COM was not close to expected value")
    enabled_capsule_COM_expected        = ("Enabled Capsule COM was close to expected value",        "Enabled Capsule COM was not close to expected value")
    enabled_box_COM_expected            = ("Enabled Box COM was close to expected value",            "Enabled Box COM was not close to expected value")
    enabled_physics_asset_COM_expected  = ("Enabled Physics Asset COM was close to expected value",  "Enabled Physics Asset COM was not close to expected value")

    disabled_sphere_COM_expected        = ("Disabled Sphere COM was close to expected value",        "Disabled Sphere COM was not close to expected value")
    disabled_capsule_COM_expected       = ("Disabled Capsule COM was close to expected value",       "Disabled Capsule COM was not close to expected value")
    disabled_box_COM_expected           = ("Disabled Box COM was close to expected value",           "Disabled Box not COM was not close to expected value")
    disabled_physics_asset_COM_expected = ("Disabled Physics Asset COM was close to expected value", "Disabled Physics Asset not COM was close to expected value")

    exit_game_mode                      = ("Exited game mode",                                       "Couldn't exit game mode")
# fmt: on


def RigidBody_COM_ComputingWorks():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure when compute COM is enabled, that the COM is automatically calculated.

    Level Description:
    There are 4 sets of entities:
        Spheres (entity: SphereEnabled), (entity: SphereDisabled)
        Boxes (entity: BoxEnabled), (entity: BoxDisabled)
        Capsules (entity: CapsuleEnabled), (entity: CapsuleDisabled)
        Physics Assets with sedan mesh (entity: PhysicsAssetEnabled), (entity: PhysicsAssetDisabled)

    Each set has two entities, both of which are identical to one another - save for the Compute COM setting on their
    rigid body. One has it set to enabled, and the other has it set to disabled.

    Each entity has a rigid body and two PhysX colliders. The first collider has no offset. The second collider is
    positioned +2 units in the X direction.

    Expected Behavior:
    The entities with 'Compute COM' disabled will have local COM offsets at the objects origin ((0.0, 0.0, 0.0) Locally)
    The entities with 'Compute COM' enabled will have local COM offsets at the average position between the two
    colliders.
        For every entity that isn't a physics asset, that's +1 unit to the right (1/2 the offset value of the second
        collider). The physics asset has a mesh whose origin is not at the natural center of mass of the object,
        so we check its COM offset as a special case.

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Validate entities
    4) Check center of mass for each entity
    5) Special Case: Check center of mass for each physics asset
    6) Exit game mode
    7) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
    - Parsing the file or running a log_monitor are required to observe the test results.

    :return: (None)
    """

    # Setup path
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as lymath

    CONTROL_COM = lymath.Vector3(0.0, 0.0, 0.0)
    EXPECTED_COM = lymath.Vector3(1.0, 0.0, 0.0)
    PHYSICS_ASSET_EXPECTED_COM = lymath.Vector3(1.0, -0.08, 0.43) # Derived from the sedan mesh specifically
    CLOSE_THRESHOLD = 0.01

    class Shape:
        def __init__(self, name, valid_test, com_valid_test):
            self.id = general.find_game_entity(name)
            self.center_of_mass = self.get_com()
            self.position = self.get_position()
            self.com_local = self.center_of_mass.Subtract(self.position)
            self.valid_test = valid_test
            self.com_valid_test = com_valid_test

        def get_com(self):
            return azlmbr.physics.RigidBodyRequestBus(bus.Event, "GetCenterOfMassWorld", self.id)

        def get_position(self):
            return azlmbr.components.TransformBus(bus.Event, "GetWorldTranslation", self.id)

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "RigidBody_COM_ComputingWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Validate entities
    sphere_enabled = Shape("SphereEnabled", Tests.find_enabled_sphere, Tests.enabled_sphere_COM_expected)
    sphere_disabled = Shape("SphereDisabled", Tests.find_disabled_sphere, Tests.disabled_sphere_COM_expected)

    capsule_enabled = Shape("CapsuleEnabled", Tests.find_enabled_capsule, Tests.enabled_capsule_COM_expected)
    capsule_disabled = Shape("CapsuleDisabled", Tests.find_disabled_capsule, Tests.disabled_capsule_COM_expected)

    box_enabled = Shape("BoxEnabled", Tests.find_enabled_box, Tests.enabled_box_COM_expected)
    box_disabled = Shape("BoxDisabled", Tests.find_disabled_box, Tests.disabled_box_COM_expected)

    physics_asset_enabled = Shape(
        "PhysicsAssetEnabled", Tests.find_enabled_physics_asset, Tests.enabled_physics_asset_COM_expected
    )
    physics_asset_disabled = Shape(
        "PhysicsAssetDisabled", Tests.find_disabled_physics_asset, Tests.disabled_physics_asset_COM_expected
    )

    # physics asset is a special case, and thus not included
    enabled_shapes = [sphere_enabled, capsule_enabled, box_enabled]
    disabled_shapes = [sphere_disabled, capsule_disabled, box_disabled]

    all_shapes = enabled_shapes + disabled_shapes + [physics_asset_enabled, physics_asset_disabled]

    for shape in all_shapes:
        Report.critical_result(shape.valid_test, shape.id.IsValid())

    # 4) Check center of mass for each entity
    for enabled_shape in enabled_shapes:
        enabled_is_close = enabled_shape.com_local.IsClose(EXPECTED_COM, CLOSE_THRESHOLD)
        Report.result(enabled_shape.com_valid_test, enabled_is_close)

    for disabled_shape in disabled_shapes:
        disabled_is_close = disabled_shape.com_local.IsClose(CONTROL_COM, CLOSE_THRESHOLD)
        Report.result(disabled_shape.com_valid_test, disabled_is_close)

    # 5) Check center of mass for each physics asset
    enabled_is_close = physics_asset_enabled.com_local.IsClose(PHYSICS_ASSET_EXPECTED_COM, CLOSE_THRESHOLD)
    print(f"{physics_asset_enabled.com_local.x}, {physics_asset_enabled.com_local.y}, {physics_asset_enabled.com_local.z}")
    Report.result(physics_asset_enabled.com_valid_test, enabled_is_close)

    disabled_is_close = physics_asset_disabled.com_local.IsClose(CONTROL_COM, CLOSE_THRESHOLD)
    Report.result(physics_asset_disabled.com_valid_test, disabled_is_close)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_COM_ComputingWorks)
