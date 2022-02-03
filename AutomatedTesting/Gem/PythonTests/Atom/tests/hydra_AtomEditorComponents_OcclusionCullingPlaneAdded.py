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
    occlusion_culling_plane_entity_creation = (
        "Occlusion Culling Plane Entity successfully created",
        "Occlusion Culling Plane Entity failed to be created")
    occlusion_culling_plane_component_added = (
        "Entity has a Occlusion Culling Plane component",
        "Entity failed to find Occlusion Culling Plane component")
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


def AtomEditorComponents_OcclusionCullingPlane_AddedToEntity():
    """
    Summary:
    Tests the occlusion culling plane component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Occlusion Culling Plane entity with no components.
    2) Add a Occlusion Culling Plane component to Occlusion Culling Plane entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Delete Occlusion Culling Plane entity.
    9) UNDO deletion.
    10) REDO deletion.
    11) Look for errors.

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
        # 1. Create a occlusion culling plane entity with no components.
        occlusion_culling_plane_entity = EditorEntity.create_editor_entity(
            AtomComponentProperties.occlusion_culling_plane())
        Report.critical_result(Tests.occlusion_culling_plane_entity_creation, 
                               occlusion_culling_plane_entity.exists())

        # 2. Add a occlusion culling plane component to occlusion culling plane entity.
        occlusion_culling_plane_component = occlusion_culling_plane_entity.add_component(
            AtomComponentProperties.occlusion_culling_plane())
        Report.critical_result(
            Tests.occlusion_culling_plane_component_added,
            occlusion_culling_plane_entity.has_component(AtomComponentProperties.occlusion_culling_plane()))

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
        Report.result(Tests.creation_undo, not occlusion_culling_plane_entity.exists())

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
        Report.result(Tests.creation_redo, occlusion_culling_plane_entity.exists())

        # 5. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        occlusion_culling_plane_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, occlusion_culling_plane_entity.is_hidden() is True)

        # 7. Test IsVisible.
        occlusion_culling_plane_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, occlusion_culling_plane_entity.is_visible() is True)

        # 8. Delete occlusion_culling_plane entity.
        occlusion_culling_plane_entity.delete()
        Report.result(Tests.entity_deleted, not occlusion_culling_plane_entity.exists())

        # 9. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, occlusion_culling_plane_entity.exists())

        # 10. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not occlusion_culling_plane_entity.exists())

        # 11. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_OcclusionCullingPlane_AddedToEntity)
