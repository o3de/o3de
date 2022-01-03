"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    camera_creation = (
        "Camera Entity successfully created",
        "Camera Entity failed to be created")
    camera_component_added = (
        "Camera component was added to entity",
        "Camera component failed to be added to entity")
    camera_component_check = (
        "Entity has a Camera component",
        "Entity failed to find Camera component")
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    exposure_control_creation = (
        "ExposureControl Entity successfully created",
        "ExposureControl Entity failed to be created")
    exposure_control_component = (
        "Entity has a Exposure Control component",
        "Entity failed to find Exposure Control component")
    exposure_control_disabled = (
        "DepthOfField component disabled",
        "DepthOfField component was not disabled.")
    post_fx_component = (
        "Entity has a Post FX Layer component",
        "Entity did not have a Post FX Layer component")
    exposure_control_enabled = (
        "DepthOfField component enabled",
        "DepthOfField component was not enabled.")
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


def AtomEditorComponents_ExposureControl_AddedToEntity():
    """
    Summary:
    Tests the Exposure Control component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Exposure Control entity with no components.
    2) Add Exposure Control component to Exposure Control entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Exposure Control component not enabled.
    6) Add Post FX Layer component since it is required by the Exposure Control component.
    7) Verify Exposure Control component is enabled.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Delete Exposure Control entity.
    12) UNDO deletion.
    13) REDO deletion.
    14) Look for errors and asserts.

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
        # 1. Creation of Exposure Control entity with no components.
        exposure_control_entity = EditorEntity.create_editor_entity(AtomComponentProperties.exposure_control())
        Report.critical_result(Tests.exposure_control_creation, exposure_control_entity.exists())

        # 2. Add Exposure Control component to Exposure Control entity.
        exposure_control_component = exposure_control_entity.add_component(AtomComponentProperties.exposure_control())
        Report.critical_result(
            Tests.exposure_control_component,
            exposure_control_entity.has_component(AtomComponentProperties.exposure_control()))

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
        Report.result(Tests.creation_undo, not exposure_control_entity.exists())

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
        Report.result(Tests.creation_redo, exposure_control_entity.exists())

        # 5. Verify Exposure Control component not enabled.
        Report.result(Tests.exposure_control_disabled, not exposure_control_component.is_enabled())

        # 6. Add Post FX Layer component since it is required by the Exposure Control component.
        exposure_control_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(Tests.post_fx_component,
                      exposure_control_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Exposure Control component is enabled.
        Report.result(Tests.exposure_control_enabled, exposure_control_component.is_enabled())

        # 8. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 9. Test IsHidden.
        exposure_control_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, exposure_control_entity.is_hidden() is True)

        # 10. Test IsVisible.
        exposure_control_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, exposure_control_entity.is_visible() is True)

        # 11. Delete ExposureControl entity.
        exposure_control_entity.delete()
        Report.result(Tests.entity_deleted, not exposure_control_entity.exists())

        # 12. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, exposure_control_entity.exists())

        # 13. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not exposure_control_entity.exists())

        # 14. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_ExposureControl_AddedToEntity)
