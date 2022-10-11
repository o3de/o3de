"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin():
    """
    Summary:
    A temporary level is created. An entity for each test case is created and added with the corresponding
    components to verify if the gradient transform is set to the world origin.

    Expected Behavior:
    1) Preview image updates to reflect change in transform of the gradient sampler.
    2) New Preview Position property is exposed, and set to 0,0,0 (world origin).
    3) Preview Size is set to 1,1,1 by default.

    Test Steps:
     1) Open level
     2) Create entity with Random Noise gradient and verify gradient position after clearing pinned entity
     3) Create entity with Levels Gradient Modifier and verify gradient position after clearing pinned entity
     4) Create entity with Posterize Gradient Modifier and verify gradient position after clearing pinned entity
     5) Create entity with Smooth-Step Gradient Modifier and verify gradient position after clearing pinned entity
     6) Create entity with Threshold Gradient Modifier and verify gradient position after clearing pinned entity
     7) Create entity with FastNoise Gradient and verify gradient position after clearing pinned entity
     8) Create entity with Dither Gradient Modifier and verify gradient position after clearing pinned entity
     9) Create entity with Invert Gradient Modifier and verify gradient position after clearing pinned entity
     10) Create entity with Perlin Noise Gradient and verify gradient position after clearing pinned entity

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import sys

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.math as math
    import azlmbr.entity as EntityId

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    WORLD_ORIGIN = math.Vector3(0.0, 0.0, 0.0)
    EXPECTED_SIZE = math.Vector3(1.0, 1.0, 1.0)
    CLOSE_THRESHOLD = sys.float_info.min

    def create_entity(entity_name, components_to_add):
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        entity = hydra.Entity(entity_name, entity_id)
        if entity_id.IsValid():
            print(f"{entity_name} entity Created")
        entity.components = []
        for component in components_to_add:
            entity.components.append(hydra.add_component(component, entity_id))
        return entity

    def clear_entityid_check_position(entity_name, components_to_add, check_preview_size=False):
        entity = create_entity(entity_name, components_to_add)
        hydra.get_set_test(entity, 0, "Previewer|Preview Settings|Pin Preview to Shape", EntityId.EntityId())
        preview_position = hydra.get_component_property_value(
            entity.components[0], "Previewer|Preview Settings|Preview Position"
        )
        preview_set_to_origin = (
            f"{entity_name}: Preview Position set to world origin",
            f"{entity_name}: Preview Position set to unexpected coords"
        )
        Report.result(preview_set_to_origin, preview_position.IsClose(WORLD_ORIGIN, CLOSE_THRESHOLD))
        if check_preview_size:
            preview_size = hydra.get_component_property_value(entity.components[0], "Previewer|Preview Settings|Preview Size")
            preview_size_default_set = (
                f"{entity_name}: Preview Size set as expected",
                f"{entity_name}: Preview Size set to unexpected value. Expected {EXPECTED_SIZE}, Found {preview_size}"
            )
            Report.result(preview_size_default_set, preview_size.IsClose(EXPECTED_SIZE, CLOSE_THRESHOLD))
        return entity

    # 1) Open an existing simple level
    hydra.open_base_level()

    # 2) Create entity with Random Noise gradient and verify gradient position after clearing pinned entity
    clear_entityid_check_position(
        "Random Noise Gradient", ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"], True
    )

    # 3) Create entity with Levels Gradient Modifier and verify gradient position after clearing pinned entity
    clear_entityid_check_position("Levels Gradient Modifier", ["Levels Gradient Modifier"])

    # 4) Create entity with Posterize Gradient Modifier and verify gradient position after clearing pinned entity
    clear_entityid_check_position("Posterize Gradient Modifier", ["Posterize Gradient Modifier"])

    # 5) Create entity with Smooth-Step Gradient Modifier and verify gradient position after clearing pinned entity
    clear_entityid_check_position("Smooth-Step Gradient Modifier", ["Smooth-Step Gradient Modifier"])

    # 6) Create entity with Threshold Gradient Modifier and verify gradient position after clearing pinned entity
    clear_entityid_check_position("Threshold Gradient Modifier", ["Threshold Gradient Modifier"])

    # 7) Create entity with FastNoise Gradient and verify gradient position after clearing pinned entity
    clear_entityid_check_position(
        "FastNoise Gradient", ["FastNoise Gradient", "Gradient Transform Modifier", "Box Shape"], True
    )

    # 8) Create entity with Dither Gradient Modifier and verify gradient position after clearing pinned entity
    clear_entityid_check_position("Dither Gradient Modifier", ["Dither Gradient Modifier"], True)

    # 9) Create entity with Invert Gradient Modifier and verify gradient position after clearing pinned entity
    clear_entityid_check_position("Invert Gradient Modifier", ["Invert Gradient Modifier"])

    # 10) Create entity with Perlin Noise Gradient and verify gradient position after clearing pinned entity
    clear_entityid_check_position(
        "Perlin Noise Gradient", ["Perlin Noise Gradient", "Gradient Transform Modifier", "Box Shape"], True
    )


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientPreviewSettings_ClearingPinnedEntitySetsPreviewToOrigin)
