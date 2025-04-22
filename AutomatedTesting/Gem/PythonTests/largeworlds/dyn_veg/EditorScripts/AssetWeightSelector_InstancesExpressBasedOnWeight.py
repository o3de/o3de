"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    highest_weight_instance_count = (
        "Found the expected number of instances when sorting by highest weight",
        "Found an unexpected number of instances when sorting by highest weight"
    )
    lowest_weight_instance_count = (
        "Found the expected number of instances when sorting by lowest weight",
        "Found an unexpected number of instances when sorting by lowest weight"
    )


def AssetWeightSelector_InstancesExpressBasedOnWeight():
    """
    Summary:
    Vegetation areas using weight selectors properly distribute instances according to Sort By Weight setting

    Expected Behavior:
    Vegetation is planted in the area according to the generated gradient pattern.
    Higher weight assets are more likely to express when "Descending (highest first)" is selected.
    Lower weight assets are more likely to express when "Ascending (lowest first)" is selected.

    Test Steps:
     1) Open a simple level
     2) Create instance spawner with 2 descriptors, one with an Empty Asset
     3) Create a planting surface
     4) Create a child entity of the instance spawner with a Constant Gradient component with default values (1.0)
     5) Pin the child entity to Vegetation Asset Weight Selector of the instance spawner entity
     6) Set first descriptor to a higher weight, and toggle off Allow Empty Assets
     7) Validate instance count with initial setup/sort values
     8) Change sort values and validate instance count

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
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

    # 2) Create a new instance spawner entity with multiple Prefab Instance Spawner descriptors, one set to a
    # valid prefab entity, and one set to None
    spawner_center_point = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "AssetWeight_PinkFlower")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0, 16.0,
                                                               pink_flower_prefab)
    desc_asset = hydra.get_component_property_value(spawner_entity.components[2],
                                                    "Configuration|Embedded Assets")[0]
    desc_list = [desc_asset, desc_asset]
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets", desc_list)
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[1]|Instance|Prefab Asset", None)

    # Add an Asset Weight Selector component to the spawner entity
    spawner_entity.add_component("Vegetation Asset Weight Selector")

    # 3) Create a planting surface
    dynveg.create_surface_entity("Planting Surface", spawner_center_point, 32.0, 32.0, 1.0)

    # 4) Create a child entity of the spawner entity with a Constant Gradient component
    components_to_add = ["Constant Gradient"]
    gradient_entity = hydra.Entity("Gradient Entity")
    gradient_entity.create_entity(spawner_center_point, components_to_add, parent_id=spawner_entity.id)

    # 5) Pin the Constant Gradient to the Vegetation Asset Weight Selector
    spawner_entity.get_set_test(3, 'Configuration|Gradient|Gradient Entity Id', gradient_entity.id)

    # 6) Set the first descriptor weight to a higher value and toggle off Allow Empty Assets on the Layer Spawner
    # component
    spawner_entity.get_set_test(2, 'Configuration|Embedded Assets|[0]|Weight', 50)
    spawner_entity.get_set_test(0, 'Configuration|Allow Empty Assets', False)

    # 7) Query for expected instances with default settings. We should have 0 instances with default Constant
    # Gradient setup sorting by higher weight first
    num_expected = 0
    initial_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.result(Tests.highest_weight_instance_count, initial_success)

    # 8) Sort by lowest weight first, and verify instance counts. We should now have 400 instances as the highest
    # priority instance won't be allowed to claim space due to "Allow Empty Assets" being False
    spawner_entity.get_set_test(3, 'Configuration|Sort By Weight', 1)
    num_expected = 20 * 20
    final_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.result(Tests.lowest_weight_instance_count, final_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AssetWeightSelector_InstancesExpressBasedOnWeight)
