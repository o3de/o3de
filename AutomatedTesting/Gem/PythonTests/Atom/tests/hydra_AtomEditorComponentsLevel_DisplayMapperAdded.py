"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    creation_undo = (
        "UNDO level component addition success",
        "UNDO level component addition failed")
    creation_redo = (
        "REDO Level component addition success",
        "REDO Level component addition failed")
    display_mapper_component = (
        "Level has a Display Mapper component",
        "Level failed to find Display Mapper component")
    ldr_color_grading_lut = (
        "LDR color Grading LUT asset set",
        "LDR color Grading LUT asset could not be set")
    enable_ldr_color_grading_lut = (
        "Enable LDR color grading LUT set",
        "Enable LDR color grading LUT could not be set")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")


def AtomEditorComponentsLevel_DisplayMapper_AddedToEntity():
    """
    Summary:
    Tests the Display Mapper level component can be added to the level entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Add Display Mapper level component to the level entity.
    2) UNDO the level component addition.
    3) REDO the level component addition.
    4) Set LDR color Grading LUT asset.
    5) Set Enable LDR color grading LUT property True
    6) Enter/Exit game mode.
    7) Look for errors and asserts.

    :return: None
    """
    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorLevelEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Add Display Mapper level component to the level entity.
        display_mapper_component = EditorLevelEntity.add_component(AtomComponentProperties.display_mapper())
        Report.critical_result(
            Tests.display_mapper_component,
            EditorLevelEntity.has_component(AtomComponentProperties.display_mapper()))

        # 2. UNDO the level component addition.
        # -> UNDO component addition.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not EditorLevelEntity.has_component(AtomComponentProperties.display_mapper()))

        # 3. REDO the level component addition.
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, EditorLevelEntity.has_component(AtomComponentProperties.display_mapper()))

        # 4. Set LDR color Grading LUT asset.
        display_mapper_asset_path = os.path.join("TestData", "test.lightingpreset.azasset")
        display_mapper_asset = Asset.find_asset_by_path(display_mapper_asset_path, False)
        display_mapper_component.set_component_property_value(
            AtomComponentProperties.display_mapper('LDR color Grading LUT'), display_mapper_asset.id)
        Report.result(
            Tests.ldr_color_grading_lut,
            display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('LDR color Grading LUT')) == display_mapper_asset.id)

        # 5. Set Enable LDR color grading LUT property True
        display_mapper_component.set_component_property_value(
            AtomComponentProperties.display_mapper('Enable LDR color grading LUT'), True)
        Report.result(
            Test.enable_ldr_color_grading_lut,
            display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Enable LDR color grading LUT')) is True)

        # 6. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 7. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponentsLevel_DisplayMapper_AddedToEntity)
