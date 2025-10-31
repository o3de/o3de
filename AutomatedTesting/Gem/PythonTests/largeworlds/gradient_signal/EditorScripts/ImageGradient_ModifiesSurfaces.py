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
    instance_validation = (
        "Found the expected number of instances",
        "Found an unexpected number of instances"
    )


def ImageGradient_ModifiesSurfaces():
    """
    Summary:
    This test verifies that an Image Gradient + Gradient Surface Tag Emitter properly modifies surfaces.

    Expected Behavior:
    Vegetation is used to verify expected surface modification.

    Test Steps:
    1) Open a level
    2) Create an entity with Image Gradient, Gradient Transform Modifier, Shape Reference, and Gradient Surface Tag
    Emitter components with an Image asset assigned.
    3) Create a Vegetation Layer Spawner setup to plant on the generated surface.
    4) Update all surface tag references
    5) Validate expected instances planted on the modified surface.

    :return: None
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.entity as EntityId
    import azlmbr.editor as editor
    import azlmbr.math as math
    import azlmbr.surface_data as surface_data
    import azlmbr.vegetation as vegetation

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    hydra.open_base_level()

    # 2) Create an entity with required Image Gradient + Surface Tag Emitter components
    components_to_add = ["Image Gradient", "Gradient Transform Modifier", "Shape Reference",
                         "Gradient Surface Tag Emitter"]
    entity_position = math.Vector3(0.0, 0.0, 0.0)
    new_entity_id = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
    )
    Report.critical_result(Tests.image_gradient_entity_created, new_entity_id.IsValid())
    image_gradient_entity = EditorEntity.create_editor_entity_at(entity_position, "Image Gradient")
    image_gradient_entity.add_components(components_to_add)

    # 3) Create vegetation and planting surface entities, and assign the Image Gradient entity's Shape Reference

    # Create vegetation entity
    purple_flower_prefab_path = os.path.join("assets", "prefabs", "PurpleFlower.spawnable")
    spawner_entity = dynveg.create_prefab_vegetation_area("Instance Spawner", entity_position, 50.0, 50.0, 10.0,
                                                          purple_flower_prefab_path)
    spawner_entity.add_component("Vegetation Surface Mask Filter")

    # Create surface entity
    dynveg.create_surface_entity("Box Shape", entity_position, 50.0, 50.0, 1.0)

    # Assign Image Gradient entity's Shape Reference
    image_gradient_entity.components[2].set_component_property_value("Configuration|Shape Entity Id", spawner_entity.id)

    # 4) Assign surface tags to the required components
    tag_list = [surface_data.SurfaceTag("terrain")]

    # Set the Veg Spawner entity's Surface Tag Mask Filter component to include the "terrain" tag
    hydra.get_set_test(spawner_entity, 3, "Configuration|Inclusion|Surface Tags", tag_list)

    # Set the Image Gradient entity's Gradient Surface Tag Emitter component to modify the "terrain" tag
    # NOTE: This requires a disable/re-enable of the component to force a refresh as assigning a tag via script does not
    grad_surf_tag_emitter_component = image_gradient_entity.components[3]
    grad_surf_tag_emitter_component.add_container_item("Configuration|Extended Tags", 0, tag_list[0])
    grad_surf_tag_emitter_component.set_enabled(False)
    grad_surf_tag_emitter_component.set_enabled(True)

    # Assign the image asset to the image gradient and validate the expected asset was set
    test_img_gradient_path = os.path.join("Assets", "ImageGradients", "image_grad_test_gsi.png.streamingimage")
    asset = Asset.find_asset_by_path(test_img_gradient_path)
    vegetation.ImageGradientRequestBus(bus.Event, "SetImageAssetPath", image_gradient_entity.id, test_img_gradient_path)
    compare_asset_path = vegetation.ImageGradientRequestBus(bus.Event, "GetImageAssetPath", image_gradient_entity.id)
    compare_asset = Asset.find_asset_by_path(compare_asset_path)
    success = compare_asset.id == asset.id
    Report.result(Tests.image_gradient_assigned, success)

    # 5) Validate the expected number of vegetation instances. Instances should only spawn on the modified surface
    num_expected_instances = 169
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
        spawner_entity.id, num_expected_instances), 5.0)
    Report.result(Tests.instance_validation, success)

    # 6) Validate there are no vegetation instances after we clear the image gradient asset
    vegetation.ImageGradientRequestBus(bus.Event, "SetImageAssetPath", image_gradient_entity.id, "")
    num_expected_instances_after_clear = 0
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(
        spawner_entity.id, num_expected_instances_after_clear), 5.0)
    Report.result(Tests.instance_validation, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ImageGradient_ModifiesSurfaces)
