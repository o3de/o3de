"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    gradient_entity_created = (
        "Successfully created new Gradient entity",
        "Failed to create Gradient entity"
    )
    non_override_rotation_check = (
        "Instances are rotated at expected values after initial setup",
        "Found unexpectedly rotated instances after initial setup"
    )
    override_rotation_check = (
        "Instances are rotated at expected values after configuring overrides",
        "Found unexpectedly rotated instances after configuring overrides"
    )


def RotationModifierOverrides_InstancesRotateWithinRange():
    """
    Summary:
    A level with simple vegetation is created. A child entity with required components is then created,
    and pinned to the gradient entity id in Z direction for vegetation entity. The changes in vegetation
    area are observed.

    Expected Behavior:
    Vegetation instances all rotate randomly between 0-360 degrees on the Z-axis.

    Test Steps:
     1) Open a new level
     2) Create vegetation entity and add components
     3) Set properties for vegetation entity
     4) Create new child entity
     5) Pin the child entity to vegetation entity as gradient entity id
     6) Verify rotation without per-item overrides
     7) Verify rotation with per-item overrides

    Note:
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.bus as bus
    import azlmbr.areasystem as areasystem

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def get_expected_rotation(min, max, gradient_value):
        return min + ((max - min) * gradient_value)

    def validate_rotation(center, radius, num_expected, rot_degrees_vector):
        # Verify that every instance in the given area has the expected rotation.
        box = math.Aabb_CreateCenterRadius(center, radius)
        instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)
        num_found = len(instances)
        num_validated = 0
        result = (num_found == num_expected)
        Report.info(f'instance count validation: {result} (found={num_found}, expected={num_expected})')
        expected_rotation = math.Quaternion()
        expected_rotation.SetFromEulerDegrees(rot_degrees_vector)
        for instance in instances:
            is_close = instance.rotation.IsClose(expected_rotation)
            result = result and is_close
            if is_close:
                num_validated = num_validated + 1
        Report.info(f'instance rotation validation: {result} (num_validated={num_validated})')
        return result

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")
    general.set_current_view_position(512.0, 480.0, 38.0)

    # 2) Create vegetation entity and add components
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Spawner Entity", entity_position, 16.0, 16.0, 16.0, asset_path)
    spawner_entity.add_component("Vegetation Rotation Modifier")
    # Our default vegetation settings places 20 instances per 16 meters, so we expect 20 * 20 total instances.
    num_expected = 20 * 20
    # This is technically twice as big as we need, but we want to make sure our query radius is large enough to discover
    # every instance we've created.
    area_radius = 16.0

    # Create surface to spawn on
    dynveg.create_surface_entity("Surface Entity", entity_position, 16.0, 16.0, 1.0)

    # 3) Set properties for the rotation override on the descriptor, but don't set "allow overrides" on the rotation
    # modifier yet
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Rotation Modifier|Override Enabled", True)
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Rotation Modifier|Min Z", -70.0)
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Rotation Modifier|Max Z", 30.0)

    # 4) Create new child entity with a constant gradient
    constant_gradient_value = 0.25
    gradient_entity = hydra.Entity("Gradient Entity")
    gradient_entity.create_entity(
        entity_position,
        ["Constant Gradient"],
        parent_id=spawner_entity.id
    )
    Report.critical_result(Tests.gradient_entity_created, gradient_entity.id.IsValid())
    gradient_entity.get_set_test(0, "Configuration|Value", constant_gradient_value)

    # 5) Pin the child entity to vegetation entity as gradient entity id
    spawner_entity.get_set_test(3, "Configuration|Rotation Z|Gradient|Gradient Entity Id", gradient_entity.id)

    # 6) Verify that without per-item overrides, the rotation matches the one calculated from the default rotation range
    general.idle_wait(1.0)
    rotation_degrees = get_expected_rotation(-180.0, 180.0, constant_gradient_value)
    rotation_success = helper.wait_for_condition(
        lambda: validate_rotation(entity_position, area_radius, num_expected, math.Vector3(0.0, 0.0, rotation_degrees)),
        5.0)
    Report.result(Tests.non_override_rotation_check, rotation_success)

    # 7) Verify that with per-item overrides enabled, the rotation matches the one calculated from the override range.
    spawner_entity.get_set_test(3, "Configuration|Allow Per-Item Overrides", True)
    rotation_degrees = get_expected_rotation(-70.0, 30.0, constant_gradient_value)
    rotation_success = helper.wait_for_condition(
        lambda: validate_rotation(entity_position, area_radius, num_expected, math.Vector3(0.0, 0.0, rotation_degrees)),
        5.0)
    Report.result(Tests.override_rotation_check, rotation_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(RotationModifierOverrides_InstancesRotateWithinRange)
