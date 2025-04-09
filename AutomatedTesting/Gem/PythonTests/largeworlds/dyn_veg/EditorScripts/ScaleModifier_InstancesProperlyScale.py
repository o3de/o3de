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
    instance_count = (
        "Found the expected number of instances",
        "Found an unexpected number of instances"
    )
    instances_properly_scaled = (
        "All instances scaled within appropriate range",
        "Found instances scaled outside of the appropriate range"
    )


def ScaleModifier_InstancesProperlyScale():
    """
    Summary:
    A New level is created. A New entity is created with components Vegetation Layer Spawner, Vegetation Asset List,
    Box Shape and Vegetation Scale Modifier. A New child entity is created with components Random Noise Gradient,
    Gradient Transform Modifier, and Box Shape. Pin the Random Noise entity to the Gradient Entity Id field for
    the Gradient group. Range Min and Range Max are set to few values and values are validated. Range Min and Range
    Max are set to few other values and values are validated.

    Expected Behavior:
    Vegetation instances are scaled within Range Min/Range Max.

    Test Steps:
     1) Open an existing level
     2) Create a new entity with components Vegetation Layer Spawner, Vegetation Asset List, Box Shape and
        Vegetation Scale Modifier
     3) Create child entity with components Random Noise Gradient, Gradient Transform Modifier and Box Shape
     4) Pin the Random Noise entity to the Gradient Entity Id field for the Gradient group.
     5) Range Min/Max is set to few different values on the Vegetation Scale Modifier component and
        scale of instances is validated

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

    def set_and_validate_scale(entity, min_scale, max_scale):
        # Set Range Min/Max
        entity.get_set_test(3, "Configuration|Range Min", min_scale)
        entity.get_set_test(3, "Configuration|Range Max", max_scale)

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
    hydra.open_base_level()

    general.set_current_view_position(500.49, 498.69, 46.66)
    general.set_current_view_rotation(-42.05, 0.00, -36.33)

    # 2) Create a new entity with components Vegetation Layer Spawner, Vegetation Asset List, Box Shape and
    # Vegetation Scale Modifier
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "ScaleMod_PinkFlower")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", entity_position, 16.0, 16.0, 16.0,
                                                               pink_flower_prefab)
    spawner_entity.add_component("Vegetation Scale Modifier")

    # Create a surface to plant on and add a Vegetation Debugger Level component to allow refreshes
    dynveg.create_surface_entity("Surface Entity", entity_position, 20.0, 20.0, 1.0)
    hydra.add_level_component("Vegetation Debugger")

    # 3) Create child entity with components Random Noise Gradient, Gradient Transform Modifier and Box Shape
    gradient_entity = hydra.Entity("Gradient Entity")
    gradient_entity.create_entity(
        entity_position,
        ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"],
        parent_id=spawner_entity.id
    )
    Report.critical_result(Tests.gradient_entity_created, gradient_entity.id.IsValid())

    # 4) Pin the Random Noise entity to the Gradient Entity Id field for the Gradient group.
    spawner_entity.get_set_test(3, "Configuration|Gradient|Gradient Entity Id", gradient_entity.id)

    # 5) Set Range Min/Max on the Vegetation Scale Modifier component to diff values, and verify instance scale is
    # within bounds
    Report.result(Tests.instances_properly_scaled, set_and_validate_scale(spawner_entity, 2.0, 4.0))
    Report.result(Tests.instances_properly_scaled, set_and_validate_scale(spawner_entity, 12.0, 40.0))
    Report.result(Tests.instances_properly_scaled, set_and_validate_scale(spawner_entity, 0.5, 2.5))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ScaleModifier_InstancesProperlyScale)
