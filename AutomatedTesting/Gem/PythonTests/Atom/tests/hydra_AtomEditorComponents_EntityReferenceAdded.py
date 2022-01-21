"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "P0: UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "P0: REDO Entity creation failed")
    entity_reference_creation = (
        "Entity Reference Entity successfully created",
        "P0: Entity Reference Entity failed to be created")
    entity_reference_component = (
        "Entity has an Entity Reference component",
        "P0: Entity failed to find Entity Reference component")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")
    entity_id_references_is_container = (
        "EntityIdReferences is a container property",
        "P1: EntityIdReferences is NOT a container property")
    container_append = (
        "EntityIdReferences append succeeded",
        "P1: EntityIdReferences append did not succeed")
    container_add = (
        "EntityIdReferences add succeeded",
        "P1: EntityIdReferences add did not succeed")
    container_update = (
        "EntityIdReferences update succeeded",
        "P1: EntityIdReferences update did not succeed")
    container_remove = (
        "EntityIdReferences remove succeeded",
        "P1: EntityIdReferences remove did not succeed")
    container_reset = (
        "EntityIdReferences reset succeeded",
        "P1: EntityIdReferences reset did not succeed")
    entity_reference_component_removed = (
        "Entity Reference component removed from entity",
        "P1: Entity Reference component NOT removed from entity")


def AtomEditorComponents_EntityReference_AddedToEntity():
    """
    Summary:
    Tests the Entity Reference component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Entity Reference entity with no components.
    2) Add Entity Reference component to Entity Reference entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) 'EntityIdReferences' is a container property
    6) Append item to 'EntityIdReferences'
    7) Add item to 'EntityIdReferences'
    8) Update item in 'EntityIdReferences'
    9) Remove item from 'EntityIdReferences'
    10) Rest the container property then put one entity reference back for further tests
    11) Remove component
    12) Undo component remove
    13) Enter/Exit game mode.
    14) Test IsHidden.
    15) Test IsVisible.
    16) Delete Entity Reference entity.
    17) UNDO deletion.
    18) REDO deletion.
    19) Look for errors.

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
        # 1. Create an Entity Reference entity with no components.
        entity_reference_entity = EditorEntity.create_editor_entity(AtomComponentProperties.entity_reference())
        Report.critical_result(Tests.entity_reference_creation, entity_reference_entity.exists())

        # 2. Add Entity Reference component to Entity Reference entity.
        entity_reference_component = entity_reference_entity.add_component(
            AtomComponentProperties.entity_reference())
        Report.critical_result(
            Tests.entity_reference_component,
            entity_reference_entity.has_component(AtomComponentProperties.entity_reference()))

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
        Report.result(Tests.creation_undo, not entity_reference_entity.exists())

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
        Report.result(Tests.creation_redo, entity_reference_entity.exists())

        # Entities for EntityIdReferences tests
        test_1 = EditorEntity.create_editor_entity('test_1')
        test_2 = EditorEntity.create_editor_entity('test_2')
        test_3 = EditorEntity.create_editor_entity('test_3')

        # 5. 'EntityIdReferences' is a container property
        Report.result(
            Tests.entity_id_references_is_container,
            entity_reference_component.is_property_container(
                AtomComponentProperties.entity_reference('EntityIdReferences')))

        # 6. Append item to 'EntityIdReferences'
        entity_reference_component.append_container_item(AtomComponentProperties.entity_reference('EntityIdReferences'),
                                                         test_1.id)
        Report.result(
            Tests.container_append,
            entity_reference_component.get_container_item(
                AtomComponentProperties.entity_reference('EntityIdReferences'), 0) == test_1.id)

        # 7. Add item to 'EntityIdReferences'
        entity_reference_component.add_container_item(AtomComponentProperties.entity_reference('EntityIdReferences'),
                                                      1, test_1.id)
        Report.result(
            Tests.container_add,
            entity_reference_component.get_container_count(
                AtomComponentProperties.entity_reference('EntityIdReferences')) == 2
        )

        # 8. Update item in 'EntityIdReferences'
        entity_reference_component.update_container_item(AtomComponentProperties.entity_reference('EntityIdReferences'),
                                                         1, test_2.id)
        Report.result(
            Tests.container_update,
            entity_reference_component.get_container_item(
                AtomComponentProperties.entity_reference('EntityIdReferences'), 1) == test_2.id)

        # 9. Remove item from 'EntityIdReferences'
        entity_reference_component.append_container_item(AtomComponentProperties.entity_reference('EntityIdReferences'),
                                                         test_3.id)
        count_before = entity_reference_component.get_container_count(
            AtomComponentProperties.entity_reference('EntityIdReferences'))
        entity_reference_component.remove_container_item(AtomComponentProperties.entity_reference('EntityIdReferences'),
                                                         1)
        count_after = entity_reference_component.get_container_count(
            AtomComponentProperties.entity_reference('EntityIdReferences'))
        Report.result(
            Tests.container_remove,
            ((count_before == 3) and (count_after == 2) and
             (entity_reference_component.get_container_item(
                 AtomComponentProperties.entity_reference('EntityIdReferences'), 1) == test_3.id))
        )

        # 10. Rest the container property then put one entity reference back for further tests
        entity_reference_component.reset_container(AtomComponentProperties.entity_reference('EntityIdReferences'))
        general.idle_wait_frames(1)
        Report.result(
            Tests.container_reset,
            entity_reference_component.get_container_count(
                AtomComponentProperties.entity_reference('EntityIdReferences')) == 0)
        entity_reference_component.append_container_item(AtomComponentProperties.entity_reference('EntityIdReferences'),
                                                         test_1.id)

        # 11. Remove component
        entity_reference_entity.remove_component(AtomComponentProperties.entity_reference())
        general.idle_wait_frames(1)
        Report.result(Tests.entity_reference_component_removed, not entity_reference_entity.has_component(
            AtomComponentProperties.entity_reference()))

        # 12. Undo component remove
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.entity_reference_component, entity_reference_entity.has_component(
            AtomComponentProperties.entity_reference()))

        # 13. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 14. Test IsHidden.
        entity_reference_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, entity_reference_entity.is_hidden() is True)

        # 15. Test IsVisible.
        entity_reference_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, entity_reference_entity.is_visible() is True)

        # 16. Delete Entity Reference entity.
        entity_reference_entity.delete()
        Report.result(Tests.entity_deleted, not entity_reference_entity.exists())

        # 17. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, entity_reference_entity.exists())

        # 18. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not entity_reference_entity.exists())

        # 19. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_EntityReference_AddedToEntity)
