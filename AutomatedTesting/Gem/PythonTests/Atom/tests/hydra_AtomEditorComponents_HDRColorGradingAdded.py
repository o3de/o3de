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
    hdr_color_grading_creation = (
        "HDR Color Grading Entity successfully created",
        "HDR Color Grading Entity failed to be created")
    hdr_color_grading_component = (
        "Entity has an HDR Color Grading component",
        "Entity failed to find HDR Color Grading component")
    hdr_color_grading_disabled = (
        "HDR Color Grading component disabled",
        "HDR Color Grading component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "Entity did not have an PostFX Layer component")
    hdr_color_grading_enabled = (
        "HDR Color Grading component enabled",
        "HDR Color Grading component was not enabled")
    enable_hdr_color_grading_parameter_enabled = (
        "Enable HDR Color Grading parameter enabled",
        "Enable HDR Color Grading parameter was not enabled")
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


def AtomEditorComponents_HDRColorGrading_AddedToEntity():
    """
    Summary:
    Tests the HDR Color Grading component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an HDR Color Grading entity with no components.
    2) Add HDR Color Grading component to HDR Color Grading entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify HDR Color Grading component not enabled.
    6) Add PostFX Layer component since it is required by the HDR Color Grading component.
    7) Verify HDR Color Grading component is enabled.
    8) Enable the "Enable HDR Color Grading" parameter.
    9) Enter/Exit game mode.
    10) Test IsHidden.
    11) Test IsVisible.
    12) Delete HDR Color Grading entity.
    13) UNDO deletion.
    14) REDO deletion.
    15) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create an HDR Color Grading entity with no components.
        hdr_color_grading_entity = EditorEntity.create_editor_entity(AtomComponentProperties.hdr_color_grading())
        Report.critical_result(Tests.hdr_color_grading_creation, hdr_color_grading_entity.exists())

        # 2. Add HDR Color Grading component to HDR Color Grading entity.
        hdr_color_grading_component = hdr_color_grading_entity.add_component(
            AtomComponentProperties.hdr_color_grading())
        Report.critical_result(
            Tests.hdr_color_grading_component,
            hdr_color_grading_entity.has_component(AtomComponentProperties.hdr_color_grading()))

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
        Report.result(Tests.creation_undo, not hdr_color_grading_entity.exists())

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
        Report.result(Tests.creation_redo, hdr_color_grading_entity.exists())

        # 5. Verify HDR Color Grading component not enabled.
        Report.result(Tests.hdr_color_grading_disabled, not hdr_color_grading_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the HDR Color Grading component.
        hdr_color_grading_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(
            Tests.postfx_layer_component,
            hdr_color_grading_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify HDR Color Grading component is enabled.
        Report.result(Tests.hdr_color_grading_enabled, hdr_color_grading_component.is_enabled())

        # 8. Enable the "Enable HDR Color Grading" parameter.
        hdr_color_grading_component.set_component_property_value(
            AtomComponentProperties.hdr_color_grading('Enable HDR color grading'), True)
        Report.result(Tests.enable_hdr_color_grading_parameter_enabled,
                      hdr_color_grading_component.get_component_property_value(
                          AtomComponentProperties.hdr_color_grading('Enable HDR color grading')) is True)

        # 9. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 10. Test IsHidden.
        hdr_color_grading_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, hdr_color_grading_entity.is_hidden() is True)

        # 11. Test IsVisible.
        hdr_color_grading_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, hdr_color_grading_entity.is_visible() is True)

        # 12. Delete HDR Color Grading entity.
        hdr_color_grading_entity.delete()
        Report.result(Tests.entity_deleted, not hdr_color_grading_entity.exists())

        # 13. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, hdr_color_grading_entity.exists())

        # 14. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not hdr_color_grading_entity.exists())

        # 15. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_HDRColorGrading_AddedToEntity)
