"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    cubemap_capture_creation = (
        "CubeMap Capture Entity successfully created",
        "P0: CubeMap Capture Entity failed to be created")
    cubemap_capture_component = (
        "Entity has a CubeMap Capture component",
        "P0: Entity failed to find CubeMap Capture component")
    cubemap_capture_component_removal = (
        "UNDO CubeMap Capture Entity success",
        "UNDO CubeMap Capture Entity failed")
    removal_undo = (
        "REDO CubeMap Capture Entity success",
        "REDO CubeMap Capture Entity failed")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO Entity deletion success",
        "P0: UNDO Entity deletion failed")
    deletion_redo = (
        "REDO Entity deletion success",
        "P0: REDO Entity deletion failed")


def AtomEditorComponents_CubeMapCapture_AddedToEntity():
    """
    Summary:
    Tests the Cubemap Capture component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, and deleted.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Cubemap_Capture entity with no components.
    2) Add Cubemap_Capture component to Cubemap_Capture entity.
    3) Remove the Cubemap_Capture component.
    4) UNDO the Cubemap_Capture component removal.
    5) Delete the Cubemap_Capture entity.
    6) UNDO deletion.
    7) REDO deletion.
    8) Look for errors.

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
        # 1. Create a Cubemap_Capture entity with no components.
        cubemap_capture_entity = EditorEntity.create_editor_entity(AtomComponentProperties.cube_map_capture())
        Report.critical_result(Tests.cubemap_capture_creation, cubemap_capture_entity.exists())

        # 2. Add Cubemap_Capture component to Cubemap_Capture entity.
        cubemap_capture_component = cubemap_capture_entity.add_component(AtomComponentProperties.cube_map_capture())
        Report.critical_result(Tests.cubemap_capture_component, cubemap_capture_entity.has_component(AtomComponentProperties.cube_map_capture()))

        # 3. Remove the Cubemap_Capture component.
        cubemap_capture_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(Tests.cubemap_capture_component_removal,
                               not cubemap_capture_entity.has_component(AtomComponentProperties.cube_map_capture()))

        # 4. Undo Cubemap_Capture component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo, cubemap_capture_entity.has_component(AtomComponentProperties.cube_map_capture()))

        # 5. Delete Cubemap_Capture entity.
        cubemap_capture_entity.delete()
        general.idle_wait_frames(1)
        Report.result(Tests.entity_deleted, not cubemap_capture_entity.exists())

        # 17. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, cubemap_capture_entity.exists())

        # 18. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not cubemap_capture_entity.exists())

        # 19. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_CubeMapCapture_AddedToEntity)
