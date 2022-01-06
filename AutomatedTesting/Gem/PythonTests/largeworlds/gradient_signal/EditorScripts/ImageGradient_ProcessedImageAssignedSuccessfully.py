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
    image_gradient_asset_found = (
        "image_grad_test_gsi.png was found in the workspace",
        "image_grad_test_gsi.png was not found in the workspace"
    )
    image_gradient_assigned = (
        "Successfully assigned image gradient asset",
        "Failed to assign image gradient asset"
    )


def ImageGradient_ProcessedImageAssignedSuccessfully():
    """
    Summary:
    Level created with Entity having Image Gradient and Gradient Transform Modifier components.
    Save any new image to your workspace with the suffix "_gsi" and assign as image asset.

    Expected Behavior:
    Image can be assigned as the Image Asset for the Image as Gradient component.

    Test Steps:
    1) Open a level
    2) Create an entity with Image Gradient and Gradient Transform Modifier components.
    3) Assign the newly processed gradient image as Image asset.

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.entity as EntityId
    import azlmbr.editor as editor
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Create an entity with Image Gradient and Gradient Transform Modifier components
    components_to_add = ["Image Gradient", "Gradient Transform Modifier", "Box Shape"]
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    new_entity_id = editor.ToolsApplicationRequestBus(
        bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
    )
    Report.critical_result(Tests.image_gradient_entity_created, new_entity_id.IsValid())
    image_gradient_entity = hydra.Entity("Image Gradient Entity", new_entity_id)
    image_gradient_entity.components = []
    for component in components_to_add:
        image_gradient_entity.add_component(component)

    # 3) Assign the processed gradient signal image as the Image Gradient's image asset and verify success

    # First, check for the base image in the workspace
    base_image = "image_grad_test_gsi.png"
    base_image_path = os.path.join("AutomatedTesting", "Assets", "ImageGradients", base_image)
    Report.critical_result(Tests.image_gradient_asset_found, os.path.isfile(base_image_path))

    # Next, assign the processed image to the Image Gradient's Image Asset property
    processed_image_path = os.path.join("Assets", "ImageGradients", "image_grad_test_gsi.gradimage")
    asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", processed_image_path, math.Uuid(),
                                            False)
    hydra.get_set_test(image_gradient_entity, 0, "Configuration|Image Asset", asset_id)

    # Finally, verify if the gradient image is assigned as the Image Asset
    success = hydra.get_component_property_value(image_gradient_entity.components[0], "Configuration|Image Asset") == asset_id
    Report.result(Tests.image_gradient_assigned, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ImageGradient_ProcessedImageAssignedSuccessfully)
