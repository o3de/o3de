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
    look_modification_creation = (
        "Look Modification Entity successfully created",
        "Look Modification Entity failed to be created")
    look_modification_component = (
        "Entity has a Look Modification component",
        "Entity failed to find Look Modification component")
    look_modification_disabled = (
        "Look Modification component disabled",
        "Look Modification component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "Entity did not have an PostFX Layer component")
    look_modification_enabled = (
        "Look Modification component enabled",
        "Look Modification component was not enabled")
    enable_look_modification_parameter_enabled = (
        "Enable look modification parameter enabled",
        "Enable look modification parameter was not enabled")
    color_grading_lut_set = (
        "Entity has the Color Grading LUT set",
        "Entity did not the Color Grading LUT set")
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
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_LookModification_AddedToEntity():
    """
    Summary:
    Tests the Look Modification component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Look Modification entity with no components.
    2) Add Look Modification component to Look Modification entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Look Modification component not enabled.
    6) Add PostFX Layer component since it is required by the Look Modification component.
    7) Verify Look Modification component is enabled.
    8) Enable the "Enable Look Modification" parameter.
    9) Add LUT asset to the Color Grading LUT parameter.
    9) Enter/Exit game mode.
    10) Test IsHidden.
    11) Test IsVisible.
    12) Delete Look Modification entity.
    13) UNDO deletion.
    14) REDO deletion.
    15) Look for errors.

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
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Look Modification entity with no components.
        look_modification_entity = EditorEntity.create_editor_entity(AtomComponentProperties.look_modification())
        Report.critical_result(Tests.look_modification_creation, look_modification_entity.exists())

        # 2. Add Look Modification component to Look Modification entity.
        look_modification_component = look_modification_entity.add_component(
            AtomComponentProperties.look_modification())
        Report.critical_result(
            Tests.look_modification_component,
            look_modification_entity.has_component(AtomComponentProperties.look_modification()))

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
        Report.result(Tests.creation_undo, not look_modification_entity.exists())

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
        Report.result(Tests.creation_redo, look_modification_entity.exists())

        # 5. Verify Look Modification component not enabled.
        Report.result(Tests.look_modification_disabled, not look_modification_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the Look Modification component.
        look_modification_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            look_modification_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Look Modification component is enabled.
        Report.result(Tests.look_modification_enabled, look_modification_component.is_enabled())

        # 8. Enable the "Enable look modification" parameter.
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('Enable look modification'), True)
        Report.result(Tests.enable_look_modification_parameter_enabled,
                      look_modification_component.get_component_property_value(
                          AtomComponentProperties.look_modification('Enable look modification')) is True)

        # 9. Set the Color Grading LUT asset on the Look Modification entity.
        color_grading_lut_path = os.path.join("ColorGrading", "TestData", "Photoshop", "inv-Log2-48nits",
                                              "test_3dl_32_lut.azasset")
        color_grading_lut_asset = Asset.find_asset_by_path(color_grading_lut_path, False)
        look_modification_component.set_component_property_value(
            AtomComponentProperties.look_modification('Color Grading LUT'), color_grading_lut_asset.id)
        Report.result(
            Tests.color_grading_lut_set,
            color_grading_lut_asset.id == look_modification_component.get_component_property_value(
                                          AtomComponentProperties.look_modification('Color Grading LUT')))

        # 10. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 11. Test IsHidden.
        look_modification_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, look_modification_entity.is_hidden() is True)

        # 12. Test IsVisible.
        look_modification_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, look_modification_entity.is_visible() is True)

        # 13. Delete Look Modification entity.
        look_modification_entity.delete()
        Report.result(Tests.entity_deleted, not look_modification_entity.exists())

        # 14. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, look_modification_entity.exists())

        # 15. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not look_modification_entity.exists())

        # 16. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_LookModification_AddedToEntity)
