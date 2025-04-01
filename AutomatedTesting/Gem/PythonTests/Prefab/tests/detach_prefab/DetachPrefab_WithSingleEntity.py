"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DetachPrefab_WithSingleEntity():

    from pathlib import Path

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():

        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general

        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from editor_python_test_tools.prefab_utils import Prefab
        from editor_python_test_tools.wait_utils import PrefabWaiter
        import Prefab.tests.PrefabTestUtils as prefab_test_utils

        TEST_PREFAB_FILE_NAME = Path(__file__).stem + '_prefab'

        prefab_test_utils.open_base_tests_level()

        # Creates an entity at the root level
        test_entity = EditorEntity.create_editor_entity("Test")
        test_prefab_entities = [test_entity]

        # Creates a prefab from the test entity
        _, test_instance = Prefab.create_prefab(test_prefab_entities, TEST_PREFAB_FILE_NAME)

        # Get parent info for the test prefab to validate hierarchy remains unchanged after Undo/Redo
        test_instance_parent = test_instance.container_entity.get_parent_id()

        # Detaches the test prefab instance
        Prefab.detach_prefab(test_instance)

        # Test undo/redo on prefab detach
        general.undo()
        PrefabWaiter.wait_for_propagation()
        is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", test_instance.container_entity.id,
                                                 azlmbr.globals.property.EditorPrefabComponentTypeId)
        test_instance_parent_undo = test_instance.container_entity.get_parent_id()
        assert test_instance_parent == test_instance_parent_undo, "Undo operation unexpectedly changed entity hierarchy"
        assert is_prefab, "Undo operation failed. Entity is not recognized as a prefab."

        general.redo()
        PrefabWaiter.wait_for_propagation()
        former_container_entity = EditorEntity.find_editor_entity(TEST_PREFAB_FILE_NAME)
        test_entity = EditorEntity.find_editor_entity("Test")
        is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", former_container_entity.id,
                                                 azlmbr.globals.property.EditorPrefabComponentTypeId)
        assert test_entity.get_parent_id() == former_container_entity.id, \
            "Redo operation unexpectedly changed entity hierarchy"
        assert not is_prefab, "Redo operation failed. Entity is still recognized as a prefab."

    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DetachPrefab_WithSingleEntity)
