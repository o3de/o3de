"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    preprocess_instance_count = (
        "Preprocess filter stage vegetation instance count is as expected",
        "Preprocess filter stage instance count found an unexpected number of instances"
    )
    postprocess_instance_count = (
        "Postprocess filter stage vegetation instance count is as expected",
        "Postprocess filter stage instance count found an unexpected number of instances"
    )


def LayerSpawner_FilterStageToggle():
    """
    Summary:
    Filter Stage toggle affects final vegetation position.

    Expected Result:
    Vegetation instances plant differently depending on the Filter Stage setting.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    PREPROCESS_INSTANCE_COUNT = 21
    POSTPROCESS_INSTANCE_COUNT = 19

    # Open an existing simple level
    hydra.open_base_level()

    general.set_current_view_position(500.49, 498.69, 46.66)
    general.set_current_view_rotation(-42.05, 0.00, -36.33)

    # Create a vegetation area with all needed components
    position = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SpawnerFilter_PinkFlower")[0]
    vegetation_entity = dynveg.create_temp_prefab_vegetation_area("vegetation", position, 16.0, 16.0, 16.0,
                                                                  pink_flower_prefab)
    vegetation_entity.add_component("Vegetation Altitude Filter")
    vegetation_entity.add_component("Vegetation Position Modifier")

    # Create a child entity under vegetation area
    child_entity = hydra.Entity("child_entity")
    components_to_add = ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"]
    child_entity.create_entity(position, components_to_add, vegetation_entity.id)

    # Set the Gradient Id in X and Y direction
    vegetation_entity.get_set_test(4, "Configuration|Position X|Gradient|Gradient Entity Id", child_entity.id)
    vegetation_entity.get_set_test(4, "Configuration|Position Y|Gradient|Gradient Entity Id", child_entity.id)

    # Set the min and max values for Altitude Filter
    vegetation_entity.get_set_test(3, "Configuration|Altitude Min", 34.0)
    vegetation_entity.get_set_test(3, "Configuration|Altitude Max", 38.0)

    # Add entity with Mesh to replicate creation of hills and a flat surface to plant on
    dynveg.create_surface_entity("Flat Surface", position, 32.0, 32.0, 1.0)
    hill_entity = dynveg.create_mesh_surface_entity_with_slopes("hill", position, 4.0)

    # Set the filter stage to preprocess and postprocess respectively and verify instance count
    vegetation_entity.get_set_test(0, "Configuration|Filter Stage", 1)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, PREPROCESS_INSTANCE_COUNT), 3.0)
    Report.result(Tests.preprocess_instance_count, result)
    vegetation_entity.get_set_test(0, "Configuration|Filter Stage", 2)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, POSTPROCESS_INSTANCE_COUNT), 3.0)
    Report.result(Tests.postprocess_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(LayerSpawner_FilterStageToggle)
