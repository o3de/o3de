"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    entities_sorted = (
        "Entities sorted in the expected order",
        "Entities sorted in an incorrect order",
    )


def EntityOutliner_EntityOrdering():
    """
    Summary:
    Verify that manual entity ordering in the entity outliner works and is stable.

    Expected Behavior:
    Several entities are created, some are manually ordered, and their order
    is maintained, even when new entities are added.

    Test Steps:
    1) Open the empty Prefab Base level
    2) Add 5 entities to the outliner
    3) Move "Entity1" to the top of the order
    4) Move "Entity4" to the bottom of the order
    5) Add another new entity, ensure the rest of the order is unchanged
    """

    import editor_python_test_tools.pyside_utils as pyside_utils
    import azlmbr.legacy.general as general
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from PySide2 import QtCore, QtWidgets, QtGui, QtTest

    # Grab the Editor, Entity Outliner, and Outliner Model
    editor_window = pyside_utils.get_editor_main_window()
    entity_outliner = pyside_utils.find_child_by_hierarchy(
        editor_window, ..., "EntityOutlinerWidgetUI", ..., "m_objectTree"
    )
    entity_outliner_model = entity_outliner.model()

    # Get the outliner index for the root prefab container entity
    def get_root_prefab_container_index():
        return entity_outliner_model.index(0, 0)

    # Get the outliner index for the top level entity of a given name
    def index_for_name(name):
        root_index = get_root_prefab_container_index()
        for row in range(entity_outliner_model.rowCount(root_index)):
            row_index = entity_outliner_model.index(row, 0, root_index)
            if row_index.data() == name:
                return row_index
        return None

    # Validate that the outliner top level entity order matches the expected order
    def verify_entities_sorted(expected_order):
        actual_order = []
        root_index = get_root_prefab_container_index()
        for row in range(entity_outliner_model.rowCount(root_index)):
            row_index = entity_outliner_model.index(row, 0, root_index)
            actual_order.append(row_index.data())

        sorted_correctly = actual_order == expected_order
        Report.result(Tests.entities_sorted, sorted_correctly)
        if not sorted_correctly:
            print(f"Expected entity order: {expected_order}")
            print(f"Actual entity order: {actual_order}")

    # Creates an entity from the outliner context menu
    def create_entity():
        pyside_utils.trigger_context_menu_entry(
            entity_outliner, "Create entity", index=get_root_prefab_container_index()
        )
        # Wait a tick after entity creation to let events process
        general.idle_wait(0.0)

    # Moves an entity (wrapped by move_entity_before and move_entity_after)
    def _move_entity(source_name, target_name, move_after=False):
        source_index = index_for_name(source_name)
        target_index = index_for_name(target_name)

        target_row = target_index.row()
        if move_after:
            target_row += 1

        # Generate MIME data and directly inject it into the model instead of
        # generating mouse click operations, as it's more reliable and we're
        # testing the underlying drag & drop logic as opposed to Qt's mouse
        # handling here
        mime_data = entity_outliner_model.mimeData([source_index])
        entity_outliner_model.dropMimeData(
            mime_data, QtCore.Qt.MoveAction, target_row, 0, target_index.parent()
        )
        # Wait after move to let events (i.e. prefab propagation) process
        general.idle_wait(1.0)

    # Move an entity before another entity in the order by dragging the source above the target
    move_entity_before = lambda source_name, target_name: _move_entity(
        source_name, target_name, move_after=False
    )
    # Move an entity after another entity in the order by dragging the source beloew the target
    move_entity_after = lambda source_name, target_name: _move_entity(
        source_name, target_name, move_after=True
    )

    expected_order = []

    # 1) Open the empty Prefab Base level
    helper.init_idle()
    helper.open_level("Prefab", "Base")

    # 2) Add 5 entities to the outliner
    ENTITIES_TO_ADD = 5
    for i in range(ENTITIES_TO_ADD):
        create_entity()

        # Our new entity should be given a name with a number automatically
        new_entity = f"Entity{i+1}"
        # The new entity should be added to the bottom of its parent entity
        expected_order = expected_order + [new_entity]

        verify_entities_sorted(expected_order)

    # 3) Move "Entity5" to the top of the order
    move_entity_before("Entity5", "Entity1")
    expected_order = ["Entity5", "Entity1", "Entity2", "Entity3", "Entity4"]
    verify_entities_sorted(expected_order)

    # 4) Move "Entity2" to the bottom of the order
    move_entity_after("Entity2", "Entity4")
    expected_order = ["Entity5", "Entity1", "Entity3", "Entity4", "Entity2"]
    verify_entities_sorted(expected_order)

    # 5) Add another new entity, ensure the rest of the order is unchanged
    create_entity()
    expected_order = ["Entity5", "Entity1", "Entity3", "Entity4", "Entity2", "Entity6"]
    verify_entities_sorted(expected_order)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(EntityOutliner_EntityOrdering)
