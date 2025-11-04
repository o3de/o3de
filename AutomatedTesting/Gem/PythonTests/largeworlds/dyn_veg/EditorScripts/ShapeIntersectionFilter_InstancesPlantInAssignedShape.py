"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    instance_count_in_box_shape = (
        "Only found instances in the configured Box Shape intersection area",
        "Found instances outside of the configured Box Shape intersection area"
    )
    instance_count_in_cylinder_shape = (
        "Only found instances in the configured Cylinder Shape intersection area",
        "Found instances outside of the configured Cylinder Shape intersection area"
    )
    unfiltered_instance_count = (
        "Found instances in the entire Spawner area with no filter set",
        "Failed to find all expected instances in the Spawner area with no filter set"
    )


def ShapeIntersectionFilter_InstancesPlantInAssignedShape():
    """
    Summary:
    A spawner area is created with a Vegetation Shape Intersection Filter. 2 different shape entities are created,
    pinned to the Shape Intersection Filter, and instance counts are verified.

    Expected Behavior:
    The Shape Entity Id reference can be successfully set/updated. Instances spawn only in the specified shape area.

    Test Steps:
     1) Open an existing level, and set view for visual debugging
     2) Create an instance spawner and planting surface
     3) Create child entity with Box Shape
     4) Create child entity with Cylinder Shape
     5) Assign the Intersection Filter to the Box Shape and validate instance counts
     6) Assign the Intersection Filter to the Cylinder Shape and validate instance counts
     7) Remove the shape reference on the Intersection Filter and validate instance counts

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    hydra.open_base_level()

    # Set view of planting area for visual debugging
    general.set_current_view_position(512.0, 500.0, 38.0)
    general.set_current_view_rotation(-20.0, 0.0, 0.0)

    # 2) Create a new entity with required vegetation area components and Vegetation Shape Intersection Filter
    center_point = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "ShapeIntersection_PinkFlower2")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", center_point, 16.0, 16.0, 1.0,
                                                               pink_flower_prefab)
    spawner_entity.add_component("Vegetation Shape Intersection Filter")

    # Create a planting surface
    dynveg.create_surface_entity("Planting Surface", center_point, 32.0, 32.0, 1.0)

    # 3) Create a child entity with Box Shape
    components_to_add = ["Box Shape"]
    box_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", spawner_entity.id)
    box = hydra.Entity("Box", box_id)
    box.components = []
    for component in components_to_add:
        box.components.append(hydra.add_component(component, box_id))
    new_box_dimension = math.Vector3(5.0, 5.0, 5.0)
    hydra.get_set_test(box, 0, "Box Shape|Box Configuration|Dimensions", new_box_dimension)

    # 4) Create a child entity with Cylinder Shape
    components_to_add = ["Cylinder Shape"]
    cylinder_id = editor.ToolsApplicationRequestBus(bus.Broadcast, "CreateNewEntity", spawner_entity.id)
    cylinder = hydra.Entity("Cylinder", cylinder_id)
    cylinder.components = []
    for component in components_to_add:
        cylinder.components.append(hydra.add_component(component, cylinder_id))
    hydra.get_set_test(cylinder, 0, "Cylinder Shape|Cylinder Configuration|Radius", 5.0)

    # 5) Set the Intersection Filter's Shape Entity Id to the Box Shape entity
    spawner_entity.get_set_test(3, "Configuration|Shape Entity Id", box_id)

    # Validate instance counts. Instances should only plant in the Box Shape area
    num_expected = 49
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.instance_count_in_box_shape, success)

    # 6) Set the Intersection Filter's Shape Entity Id to the Cylinder Shape entity
    spawner_entity.get_set_test(3, "Configuration|Shape Entity Id", cylinder_id)

    # Validate instance counts. Instances should only plant in the Cylinder Shape area
    num_expected = 121
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.instance_count_in_cylinder_shape, success)

    # 7) Clear the Intersection Filter's Shape Entity Id reference
    spawner_entity.get_set_test(3, "Configuration|Shape Entity Id", None)

    # Validate instance counts. Instances should now fill the entire spawner_entity's area
    num_expected = 20 * 20
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                             num_expected), 5.0)
    Report.result(Tests.unfiltered_instance_count, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ShapeIntersectionFilter_InstancesPlantInAssignedShape)
