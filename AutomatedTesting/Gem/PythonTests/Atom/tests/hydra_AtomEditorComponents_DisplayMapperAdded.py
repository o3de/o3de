"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    display_mapper_creation = (
        "Display Mapper Entity successfully created",
        "Display Mapper Entity failed to be created")
    display_mapper_component = (
        "Entity has a Display Mapper component",
        "Entity failed to find Display Mapper component")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    ldr_color_grading_lut = (
        "LDR color Grading LUT asset set",
        "LDR color Grading LUT asset could not be set")
    enable_ldr_color_grading_lut = (
        "Enable LDR color grading LUT set",
        "Enable LDR color grading LUT could not be set")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_DisplayMapper_AddedToEntity():
    """
    Summary:
    Tests the Display Mapper component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Display Mapper entity with no components.
    2) Add Display Mapper component to Display Mapper entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Set LDR color Grading LUT asset.
    6) Set Enable LDR color grading LUT property True
    7) Enter/Exit game mode.
    8) Test IsHidden.
    9) Test IsVisible.
    10) Delete Display Mapper entity.
    11) UNDO deletion.
    12) REDO deletion.
    13) Look for errors and asserts.

    :return: None
    """
    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Display Mapper entity with no components.
        display_mapper_entity = EditorEntity.create_editor_entity(AtomComponentProperties.display_mapper())
        Report.critical_result(Tests.display_mapper_creation, display_mapper_entity.exists())

        # 2. Add Display Mapper component to Display Mapper entity.
        display_mapper_component = display_mapper_entity.add_component(AtomComponentProperties.display_mapper())
        Report.critical_result(
            Tests.display_mapper_component,
            display_mapper_entity.has_component(AtomComponentProperties.display_mapper()))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not display_mapper_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, display_mapper_entity.exists())

        # 5. Set LDR color Grading LUT asset.
        display_mapper_asset_path = os.path.join("TestData", "test.lightingpreset.azasset")
        display_mapper_asset = Asset.find_asset_by_path(display_mapper_asset_path, False)
        display_mapper_component.set_component_property_value(
            AtomComponentProperties.display_mapper("LDR color Grading LUT"), display_mapper_asset.id)
        Report.result(
            Tests.ldr_color_grading_lut,
            display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper("LDR color Grading LUT")) == display_mapper_asset.id)

        # 6. Set Enable LDR color grading LUT property True
        display_mapper_component.set_component_property_value(
            AtomComponentProperties.display_mapper('Enable LDR color grading LUT'), True)
        Report.result(
            Tests.enable_ldr_color_grading_lut,
            display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Enable LDR color grading LUT')) is True)

        # 7. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 8. Test IsHidden.
        display_mapper_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, display_mapper_entity.is_hidden() is True)

        # 9. Test IsVisible.
        display_mapper_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, display_mapper_entity.is_visible() is True)

        # 10. Delete Display Mapper entity.
        display_mapper_entity.delete()
        Report.result(Tests.entity_deleted, not display_mapper_entity.exists())

        # 11. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, display_mapper_entity.exists())

        # 12. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not display_mapper_entity.exists())

        # 13. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DisplayMapper_AddedToEntity)
