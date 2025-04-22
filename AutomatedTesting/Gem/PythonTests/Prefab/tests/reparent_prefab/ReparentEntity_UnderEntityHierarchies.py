"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def ReparentEntity_UnderEntityHierarchies():

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        """
        Tests reparenting of an entity under various entity hierarchies:

        1. Reparents an entity under another entity
        2. Reparents an entity under a large nested entity hierarchy
        3. Validates Undo/Redo functionality for each use case
        """
        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from editor_python_test_tools.wait_utils import PrefabWaiter
        import Prefab.tests.PrefabTestUtils as prefab_test_utils

        import azlmbr.legacy.general as general
        import azlmbr.math as math

        async def reparent_entity_with_undo_redo(entity_to_reparent, new_parent):
            # Get id for original parent
            original_parent = EditorEntity(entity_to_reparent.get_parent_id())

            # Reparent to the new parent
            entity_to_reparent.set_parent_entity(new_parent.id)

            # Undo the reparent operation, and verify original parent is restored
            general.undo()
            PrefabWaiter.wait_for_propagation()
            original_parent_children_ids = original_parent.get_children_ids()
            new_parent_children_ids = new_parent.get_children_ids()
            assert entity_to_reparent.id in original_parent_children_ids, \
                "Undo failed: Failed to find entity as a child of the original parent."
            assert entity_to_reparent.id not in new_parent_children_ids, \
                "Undo failed: Unexpectedly still found entity as a child of the new parent."

            # Redo the reparent operation, and verify the new instance is not among the original parent's child entities
            general.redo()
            PrefabWaiter.wait_for_propagation()
            original_parent_children_ids = original_parent.get_children_ids()
            new_parent_children_ids = new_parent.get_children_ids()
            assert entity_to_reparent.id not in original_parent_children_ids, \
                "Redo failed: Unexpectedly found entity as a child of the original parent."
            assert entity_to_reparent.id in new_parent_children_ids, \
                "Redo failed: Failed to find entity as a child of the new parent."

        prefab_test_utils.open_base_tests_level()

        # Creates 2 new test entities at the root level
        test_entity = EditorEntity.create_editor_entity("TestEntity")
        single_parent_entity = EditorEntity.create_editor_entity("SingleParent")

        # Creates a large nested hierarchy of entities
        nested_hierarchy_pos = math.Vector3(0.0, 0.0, 0.0)
        nested_hierarchy_root_entity = prefab_test_utils.create_linear_nested_entities("NestedEntity", 50,
                                                                                       nested_hierarchy_pos)

        # Reparents the test entity under the single parent entity
        await reparent_entity_with_undo_redo(test_entity, single_parent_entity)

        # Reparents the test entity under the nested hierarchy's root entity
        await reparent_entity_with_undo_redo(test_entity, nested_hierarchy_root_entity)

        # Find an entity within the large entity hierarchy, and reparent the test entity to it
        nested_entity = EditorEntity.find_editor_entity("NestedEntity33")
        await reparent_entity_with_undo_redo(test_entity, nested_entity)

    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ReparentEntity_UnderEntityHierarchies)
