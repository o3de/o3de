"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    gradient_entity_created = (
        "Successfully created Gradient entity",
        "Failed to create Gradient entity"
    )
    scale_values_set = (
        "Scale Min and Scale Max are set to 0.1 and 1.0 in Vegetation Asset List",
        "Scale Min and Scale Max are not set to 0.1 and 1.0 in Vegetation Asset List"
    )
    instance_count = (
        "Found the expected number of instances",
        "Found an unexpected number of instances"
    )
    instances_properly_scaled = (
        "All instances scaled within appropriate range",
        "Found instances scaled outside of the appropriate range"
    )


def ScaleModifierOverrides_InstancesProperlyScale():
    """
    Summary:
    A level is created, then as simple vegetation area is created. Vegetation Scale Modifier component is
    added to the vegetation area. A new child entity is created with Random Noise Gradient Generator,
    Gradient Transform Modifier, and Box Shape. Child entity is set as gradient entity id in Vegetation
    Scale Modifier, and scale of instances is validated to fall within expected range.

    Expected Behavior:
    Vegetation instances have random scale between Range Min and Range Max applied.

    Test Steps:
     1) Open an existing level
     2) Create a new entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
     3) Set a valid mesh asset on the Vegetation Asset List
     4) Add Vegetation Scale Modifier component to the vegetation and set the values
     5) Toggle on Scale Modifier Override and verify Scale Min and Scale Max are set 0.1 and 1.0
     6) Create a new child entity and add components
     7) Add child entity as gradient entity id in Vegetation Scale Modifier
     8) Validate scale of instances with a few different min/max override values

    Note:
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.areasystem as areasystem
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Constants
    CLOSE_ENOUGH_THRESHOLD = 0.01

    def set_and_validate_scale(entity, min_scale, max_scale):
        # Set Range Min/Max
        entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Scale Modifier|Min", min_scale)
        entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Scale Modifier|Max", max_scale)

        # Refresh the planted instances
        general.run_console("veg_DebugClearAllAreas")

        # Check the initial instance count
        num_expected = 20 * 20
        success = helper.wait_for_condition(
            lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
        Report.critical_result(Tests.instance_count, success)

        # Validate scale values of instances
        box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', entity.id)
        instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)
        for instance in instances:
            if min_scale <= instance.scale <= max_scale:
                return True
        return False

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    general.set_current_view_position(500.49, 498.69, 46.66)
    general.set_current_view_rotation(-42.05, 0.00, -36.33)

    # 2) Create a new entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Spawner Entity", entity_position, 16.0, 16.0, 10.0, asset_path)

    # Create a surface to plant on and add a Vegetation Debugger Level component to allow refreshes
    dynveg.create_surface_entity("Surface Entity", entity_position, 20.0, 20.0, 1.0)
    hydra.add_level_component("Vegetation Debugger")

    # 4) Add Vegetation Scale Modifier component to the vegetation and set the values
    spawner_entity.add_component("Vegetation Scale Modifier")
    spawner_entity.get_set_test(3, "Configuration|Allow Per-Item Overrides", True)

    # 5) Toggle on Scale Modifier Override and verify Scale Min and Scale Max are set 0.1 and 1.0
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Scale Modifier|Override Enabled", True)
    scale_min = float(
        format(
            (
                hydra.get_component_property_value(
                    spawner_entity.components[2], "Configuration|Embedded Assets|[0]|Scale Modifier|Min"
                )
            ),
            ".1f",
        )
    )
    scale_max = float(
        format(
            (
                hydra.get_component_property_value(
                    spawner_entity.components[2], "Configuration|Embedded Assets|[0]|Scale Modifier|Max"
                )
            ),
            ".1f",
        )
    )
    Report.result(Tests.scale_values_set, ((scale_max - 1.0) < CLOSE_ENOUGH_THRESHOLD) and
                  ((scale_min - 0.1) < CLOSE_ENOUGH_THRESHOLD))

    # 6) Create a new child entity and add components
    gradient_entity = hydra.Entity("Gradient Entity")
    gradient_entity.create_entity(
        entity_position,
        ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"],
        parent_id=spawner_entity.id
    )
    Report.result(Tests.gradient_entity_created, gradient_entity.id.IsValid())

    # 7) Add child entity as gradient entity id in Vegetation Scale Modifier
    spawner_entity.get_set_test(3, "Configuration|Gradient|Gradient Entity Id", gradient_entity.id)

    # 8) Validate instances are scaled properly via a few different Range Min/Max settings on the override
    Report.result(Tests.instances_properly_scaled, set_and_validate_scale(spawner_entity, 0.1, 1.0))
    Report.result(Tests.instances_properly_scaled, set_and_validate_scale(spawner_entity, 2.0, 2.5))
    Report.result(Tests.instances_properly_scaled, set_and_validate_scale(spawner_entity, 1.0, 5.0))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ScaleModifierOverrides_InstancesProperlyScale)
