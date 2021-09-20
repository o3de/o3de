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
    preprocess_instance_count = (
        "Found the expected number of instances with preprocessing filter stage",
        "Found an unexpected number of instances with preprocessing filter stage"
    )
    postprocess_instance_count = (
        "Found the expected number of instances with postprocessing filter stage",
        "Found an unexpected number of instances with postprocessing filter stage"
    )


def ShapeIntersectionFilter_FilterStageToggle():
    """
    Summary:
    Filter Stage toggle affects final vegetation position

    Expected Result:
    Vegetation instances plant differently depending on the Filter Stage setting. With PreProcess, some vegetation
    instances can appear on slopes outside the filtered values. With PostProcess, vegetation instances only appear on
    the correct slope values.

    :return: None
    """

    import os

    import azlmbr.math as math
    import azlmbr.legacy.general as general

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    general.set_current_view_position(512.0, 480.0, 38.0)

    # Create basic vegetation entity
    position = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    vegetation = dynveg.create_vegetation_area("vegetation", position, 16.0, 16.0, 16.0, asset_path)

    # Create Surface for instances to plant on
    dynveg.create_surface_entity("Surface_Entity_Parent", position, 16.0, 16.0, 1.0)

    # Add a Vegetation Shape Intersection Filter to the vegetation area entity
    vegetation.add_component("Vegetation Shape Intersection Filter")

    # Create a new entity as a child of the vegetation area entity with Box Shape
    box = hydra.Entity("box")
    box.create_entity(position, ["Box Shape"])
    box.get_set_test(0, "Box Shape|Box Configuration|Dimensions", math.Vector3(8.0, 8.0, 1.0))

    # Create a new entity as a child of the vegetation area entity with Cylinder Shape.
    cylinder = hydra.Entity("cylinder")
    cylinder.create_entity(position, ["Cylinder Shape"])
    cylinder.get_set_test(0, "Cylinder Shape|Cylinder Configuration|Radius", 5.0)
    cylinder.get_set_test(0, "Cylinder Shape|Cylinder Configuration|Height", 5.0)
    box.set_test_parent_entity(vegetation)
    cylinder.set_test_parent_entity(vegetation)

    # On the Shape Intersection Filter component, click the crosshair button, and add child entities one by one
    vegetation.get_set_test(3, "Configuration|Shape Entity Id", box.id)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 8.0, 100), 2.0)
    Report.result(Tests.instance_count_in_box_shape, result)
    vegetation.get_set_test(3, "Configuration|Shape Entity Id", cylinder.id)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 5.0, 100), 2.0)
    Report.result(Tests.instance_count_in_cylinder_shape, result)

    # Create a new entity as a child of the area entity with Random Noise Gradient, Gradient Transform Modifier,
    # and Box Shape component
    random_noise = hydra.Entity("random_noise")
    random_noise.create_entity(position, ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"])
    random_noise.set_test_parent_entity(vegetation)

    # Add a Vegetation Position Modifier to the vegetation area entity
    vegetation.add_component("Vegetation Position Modifier")

    # Pin the Random Noise entity to the Gradient Entity Id field of the Position Modifier's Gradient X
    vegetation.get_set_test(4, "Configuration|Position X|Gradient|Gradient Entity Id", random_noise.id)

    # Toggle between PreProcess and PostProcess
    vegetation.get_set_test(3, "Configuration|Filter Stage", 1)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 5.0, 117), 2.0)
    Report.result(Tests.preprocess_instance_count, result)
    vegetation.get_set_test(3, "Configuration|Filter Stage", 2)
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 5.0, 122), 2.0)
    Report.result(Tests.postprocess_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ShapeIntersectionFilter_FilterStageToggle)
