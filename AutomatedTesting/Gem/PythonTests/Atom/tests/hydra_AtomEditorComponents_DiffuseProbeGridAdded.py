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
    diffuse_probe_grid_creation = (
        "Diffuse Probe Grid Entity successfully created",
        "Diffuse Probe Grid Entity failed to be created")
    diffuse_probe_grid_component = (
        "Entity has a Diffuse Probe Grid component",
        "Entity failed to find Diffuse Probe Grid component")
    diffuse_probe_grid_disabled = (
        "Diffuse Probe Grid component disabled",
        "Diffuse Probe Grid component was not disabled")
    diffuse_probe_grid_enabled = (
        "Diffuse Probe Grid component enabled",
        "Diffuse Probe Grid component was not enabled")
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


def AtomEditorComponents_DiffuseProbeGrid_AddedToEntity():
    """
    Summary:
    Tests the Diffuse Probe Grid component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Diffuse Probe Grid entity with no components.
    2) Add a Diffuse Probe Grid component to Diffuse Probe Grid entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Diffuse Probe Grid component not enabled.
    6) Add Shape component since it is required by the Diffuse Probe Grid component.
    7) Verify Diffuse Probe Grid component is enabled.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Delete Diffuse Probe Grid entity.
    12) UNDO deletion.
    13) REDO deletion.
    14) Look for errors.

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
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Diffuse Probe Grid entity with no components.
        diffuse_probe_grid_entity = EditorEntity.create_editor_entity(AtomComponentProperties.diffuse_probe_grid())
        Report.critical_result(Tests.diffuse_probe_grid_creation, diffuse_probe_grid_entity.exists())

        # 2. Add a Diffuse Probe Grid component to Diffuse Probe Grid entity.
        diffuse_probe_grid_component = diffuse_probe_grid_entity.add_component(
            AtomComponentProperties.diffuse_probe_grid())
        Report.critical_result(
            Tests.diffuse_probe_grid_component,
            diffuse_probe_grid_entity.has_component(AtomComponentProperties.diffuse_probe_grid()))

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
        Report.result(Tests.creation_undo, not diffuse_probe_grid_entity.exists())

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
        Report.result(Tests.creation_redo, diffuse_probe_grid_entity.exists())

        # 5. Verify Diffuse Probe Grid component not enabled.
        Report.result(Tests.diffuse_probe_grid_disabled, not diffuse_probe_grid_component.is_enabled())

        # 6. Add Shape component since it is required by the Diffuse Probe Grid component.
        for shape in AtomComponentProperties.diffuse_probe_grid('shapes'):
            diffuse_probe_grid_entity.add_component(shape)
            test_shape = (
                f"Entity has a {shape} component",
                f"Entity did not have a {shape} component")
            Report.result(test_shape, diffuse_probe_grid_entity.has_component(shape))

            # 7. Check if required shape allows Diffuse Probe Grid to be enabled
            Report.result(Tests.diffuse_probe_grid_enabled, diffuse_probe_grid_component.is_enabled())

            # Undo to remove each added shape except the last one and verify Diffuse Probe Grid is not enabled.
            if not (shape == AtomComponentProperties.diffuse_probe_grid('shapes')[-1]):
                general.undo()
                TestHelper.wait_for_condition(lambda: not diffuse_probe_grid_entity.has_component(shape), 1.0)
                Report.result(Tests.diffuse_probe_grid_disabled, not diffuse_probe_grid_component.is_enabled())

        # 8. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 9. Test IsHidden.
        diffuse_probe_grid_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, diffuse_probe_grid_entity.is_hidden() is True)

        # 10. Test IsVisible.
        diffuse_probe_grid_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, diffuse_probe_grid_entity.is_visible() is True)

        # 11. Delete Diffuse Probe Grid entity.
        diffuse_probe_grid_entity.delete()
        Report.result(Tests.entity_deleted, not diffuse_probe_grid_entity.exists())

        # 12. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, diffuse_probe_grid_entity.exists())

        # 13. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not diffuse_probe_grid_entity.exists())

        # 14. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DiffuseProbeGrid_AddedToEntity)
