"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    preprocess_instance_count = (
        "Pre-process instance counts are accurate",
        "Unexpected number of pre-process instances found"
    )
    postprocess_instance_count = (
        "Post-process instance counts are accurate",
        "Unexpected number of post-process instances found"
    )


def AltitudeFilter_FilterStageToggle():
    """
    Summary:
    Filter Stage toggle affects final vegetation position

    Expected Result:
    Vegetation instances plant differently depending on the Filter Stage setting.
    PostProcess should cause some number of plants that appear above and below the desired altitude range to disappear.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    PREPROCESS_INSTANCE_COUNT = 44
    POSTPROCESS_INSTANCE_COUNT = 34

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")
    general.set_current_view_position(512.0, 480.0, 38.0)

    # Create basic vegetation entity
    position = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    vegetation = dynveg.create_vegetation_area("vegetation", position, 16.0, 16.0, 16.0, asset_path)

    # Add a Vegetation Altitude Filter to the vegetation area entity
    vegetation.add_component("Vegetation Altitude Filter")

    # Create Surface for instances to plant on
    dynveg.create_surface_entity("Surface_Entity_Parent", position, 16.0, 16.0, 1.0)

    # Add entity with Mesh to replicate creation of hills
    hill_entity = dynveg.create_mesh_surface_entity_with_slopes("hill", position, 10.0)

    # Set a Min Altitude of 38 and Max of 40 in Vegetation Altitude Filter
    vegetation.get_set_test(3, "Configuration|Altitude Min", 38.0)
    vegetation.get_set_test(3, "Configuration|Altitude Max", 40.0)

    # Create a new entity as a child of the vegetation area entity with Random Noise Gradient Generator, Gradient
    # Transform Modifier, and Box Shape component
    random_noise = hydra.Entity("random_noise")
    random_noise.create_entity(position, ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"])
    random_noise.set_test_parent_entity(vegetation)

    # Add a Vegetation Position Modifier to the vegetation area entity.
    vegetation.add_component("Vegetation Position Modifier")

    # Pin the Random Noise entity to the Gradient Entity Id field of the Position Modifier's Gradient X
    vegetation.get_set_test(4, "Configuration|Position X|Gradient|Gradient Entity Id", random_noise.id)

    # Toggle between PreProcess and PostProcess in Vegetation Altitude Filter
    vegetation.get_set_test(3, "Configuration|Filter Stage", 1)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 30.0, PREPROCESS_INSTANCE_COUNT), 2.0)
    Report.result(Tests.preprocess_instance_count, result)
    vegetation.get_set_test(3, "Configuration|Filter Stage", 2)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 30.0, POSTPROCESS_INSTANCE_COUNT), 2.0)
    Report.result(Tests.postprocess_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(AltitudeFilter_FilterStageToggle)
