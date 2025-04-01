"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    image_gradient_entity_created = (
        "Image Gradient entity created",
        "Failed to create Image Gradient entity",
    )
    image_gradient_assigned = (
        "Successfully assigned image gradient asset",
        "Failed to assign image gradient asset"
    )
    point_instance_validation = (
        "Found the expected number of instances with sampling type set to Point",
        "Found an unexpected number of instances with sampling type set to Point"
    )
    bilinear_instance_validation = (
        "Found the expected number of instances with sampling type set to Bilinear",
        "Found an unexpected number of instances with sampling type set to Bilinear"
    )


def ImageGradient_BilinearFiltering():
    """
    Summary:
    This test verifies that an Image Gradient can be sampled bilinearly.

    Expected Behavior:
    Vegetation is used to verify expected surface modification.

    Test Steps:
    1) Open a level
    2) Add a Vegetation System Settings level component and adjust instances per meter to 1, and snap mode to Center
    3) Create an entity with Image Gradient, Gradient Transform Modifier, and Box Shape components with an Image asset
    assigned.
    4) Create a Vegetation Layer Spawner setup to plant on the generated surface.
    5) Validate expected instances planted on the modified surface via point sampling
    6) Validate expected instances planted on the modified surface via bilinear sampling

    :return: None
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.math as math
    import azlmbr.vegetation as vegetation

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity, EditorLevelEntity
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    hydra.open_base_level()

    # 2) Add a Vegetation System Settings component and set instance spawning to 1 instance per meter
    veg_settings_component = EditorLevelEntity.add_component("Vegetation System Settings")
    veg_settings_component.set_component_property_value("Configuration|Area System Settings|Sector Size In Meters",
                                                        20.0)
    veg_settings_component.set_component_property_value("Configuration|Area System Settings|Sector Point Snap Mode", 1)

    # 3) Create an entity with required Image Gradient components and configure
    components_to_add = ["Image Gradient", "Gradient Transform Modifier", "Box Shape"]
    entity_position = math.Vector3(0.0, 0.0, 0.0)
    image_gradient_entity = EditorEntity.create_editor_entity_at(entity_position, "Image Gradient")
    Report.critical_result(Tests.image_gradient_entity_created, image_gradient_entity.id.IsValid())
    image_gradient_entity.add_components(components_to_add)

    # Resize Image Gradient entity's Box Shape and set the Wrapping Type to Clamp to Edge
    image_gradient_box_size = math.Vector3(64.0, 64.0, 1.0)
    image_gradient_entity.components[2].set_component_property_value("Box Shape|Box Configuration|Dimensions",
                                                                     image_gradient_box_size)
    image_gradient_entity.components[1].set_component_property_value("Configuration|Wrapping Type", 1)

    # Assign the image asset to the image gradient and validate the expected asset was set
    test_img_gradient_path = os.path.join("assets", "imagegradients", "black_white_gsi.png.streamingimage")
    asset = Asset.find_asset_by_path(test_img_gradient_path)
    vegetation.ImageGradientRequestBus(bus.Event, "SetImageAssetPath", image_gradient_entity.id, test_img_gradient_path)
    compare_asset_path = vegetation.ImageGradientRequestBus(bus.Event, "GetImageAssetPath", image_gradient_entity.id)
    compare_asset = Asset.find_asset_by_path(compare_asset_path)
    success = compare_asset.id == asset.id
    Report.result(Tests.image_gradient_assigned, success)

    # 4) Create vegetation and planting surface entities and configure

    # Create vegetation entity with a Distribution Filter pinned to the gradient entity with a .01 Min Threshold
    purple_flower_prefab_path = os.path.join("assets", "prefabs", "PurpleFlower.spawnable")
    spawner_entity = dynveg.create_prefab_vegetation_area("Instance Spawner", entity_position, 64.0, 1.0, 10.0,
                                                          purple_flower_prefab_path)
    spawner_entity.add_component("Vegetation Distribution Filter")
    hydra.get_property_tree(spawner_entity.components[3])
    hydra.get_set_test(spawner_entity, 3, "Configuration|Threshold Min", 0.01)
    hydra.get_set_test(spawner_entity, 3, "Configuration|Gradient|Gradient Entity Id", image_gradient_entity.id)

    # Create surface entity
    surface_entity_position = math.Vector3(0.0, 0.5, 0.0)
    dynveg.create_surface_entity("Box Shape", surface_entity_position, 64.0, 1.0, 1.0)

    # 5) Validate the expected number of vegetation instances when sampling via "Point". Instances should fill exactly
    # half of the spawner's Box Size on X
    num_expected_instances_point = 32
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
        spawner_entity.id, num_expected_instances_point), 5.0)
    Report.result(Tests.point_instance_validation, success)

    # 6) Validate the expected number of vegetation instances when sampling via "Bilinear". Instances should fill
    # exactly 25% more of the spawner's Box Size on X than when sampling via Point
    image_gradient_entity.components[0].set_component_property_value("Configuration|Sampling Type", 1)
    num_expected_instances_bilinear = 48
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
        spawner_entity.id, num_expected_instances_bilinear), 5.0)
    Report.result(Tests.bilinear_instance_validation, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ImageGradient_BilinearFiltering)
