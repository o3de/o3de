"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def DetachPrefab_WithNestedEntities():
    from pathlib import Path

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general
        import azlmbr.math as math

        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from editor_python_test_tools.prefab_utils import Prefab
        from editor_python_test_tools.wait_utils import PrefabWaiter
        import Prefab.tests.PrefabTestUtils as prefab_test_utils

        TEST_PREFAB_FILE_NAME = Path(__file__).stem + '_prefab1'
        NESTED_ENTITIES_NAME_PREFIX = 'Entity_'
        NUM_NESTED_ENTITIES_LEVELS = 10
        POSITION = math.Vector3(0.0, 0.0, 0.0)

        prefab_test_utils.open_base_tests_level()

        # Creates new nested entities at the root level. Asserts if creation didn't succeed
        nested_entities_root = prefab_test_utils.create_linear_nested_entities(NESTED_ENTITIES_NAME_PREFIX,
                                                                               NUM_NESTED_ENTITIES_LEVELS, POSITION)
        level_entity = nested_entities_root.get_parent_id()
        prefab_test_utils.validate_linear_nested_entities(nested_entities_root, NUM_NESTED_ENTITIES_LEVELS,
                                                          POSITION)

        # Creates a prefab from the nested entities. Asserts if prefab creation doesn't succeed
        _, nested_entities_prefab_instance = Prefab.create_prefab([nested_entities_root],
                                                                  TEST_PREFAB_FILE_NAME)
        nested_entities_root_on_instance = nested_entities_prefab_instance.get_direct_child_entities()[0]
        prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS,
                                                          POSITION)

        # Detaches the nested entities prefab instance
        Prefab.detach_prefab(nested_entities_prefab_instance)

        # Test undo/redo on prefab detach
        general.undo()
        PrefabWaiter.wait_for_propagation()
        is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType",
                                                 nested_entities_prefab_instance.container_entity.id,
                                                 azlmbr.globals.property.EditorPrefabComponentTypeId)
        assert is_prefab, "Undo operation failed. Entity hierarchy is not recognized as a prefab."
        prefab_test_utils.validate_linear_nested_entities(nested_entities_root_on_instance, NUM_NESTED_ENTITIES_LEVELS,
                                                          POSITION)

        general.redo()
        PrefabWaiter.wait_for_propagation()
        former_container_entity = EditorEntity.find_editor_entity(TEST_PREFAB_FILE_NAME)
        root_entity = EditorEntity.find_editor_entity("Entity_0")
        is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", former_container_entity.id,
                                                 azlmbr.globals.property.EditorPrefabComponentTypeId)
        prefab_test_utils.validate_linear_nested_entities(root_entity, NUM_NESTED_ENTITIES_LEVELS, POSITION)
        assert not is_prefab, "Redo operation failed. Entity is still recognized as a prefab."

    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(DetachPrefab_WithNestedEntities)
