"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def ReparentPrefab_UnderPrefabAndEntityHierarchies():

    from pathlib import Path
    CAR_PREFAB_FILE_NAME = Path(__file__).stem + '_car_prefab'
    WHEEL_PREFAB_FILE_NAME = Path(__file__).stem + '_wheel_prefab'
    DRIVER_PREFAB_FILE_NAME = Path(__file__).stem + '_driver_prefab'

    import editor_python_test_tools.pyside_utils as pyside_utils

    @pyside_utils.wrap_async
    async def run_test():

        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from editor_python_test_tools.prefab_utils import Prefab, wait_for_propagation

        import Prefab.tests.PrefabTestUtils as prefab_test_utils
        import azlmbr.legacy.general as general

        async def reparent_with_undo_redo(prefab_file_name, prefab_instance, new_parent_entity_id):
            # Get ids for original parent, original container id, and new parent
            original_parent = EditorEntity(prefab_instance.container_entity.get_parent_id())
            original_container_id = prefab_instance.container_entity.id
            new_parent = EditorEntity(new_parent_entity_id)

            # Reparent to the new parent
            await prefab_instance.ui_reparent_prefab_instance(new_parent_entity_id)

            # Undo the reparent operation, and verify original parent is restored
            general.undo()
            wait_for_propagation()
            original_parent_children_ids = original_parent.get_children_ids()
            new_parent_children_ids = new_parent.get_children_ids()
            instance_id = general.find_editor_entity(prefab_file_name)
            assert instance_id in original_parent_children_ids, \
                "Undo failed: Failed to find instance as a child of the original parent."
            assert instance_id not in new_parent_children_ids, \
                "Undo failed: Unexpectedly still found instance as a child of the new parent."

            # Redo the reparent operation, and verify the new instance is not among the original parent's child entities
            general.redo()
            wait_for_propagation()
            original_parent_children_ids = original_parent.get_children_ids()
            new_parent_children_ids = new_parent.get_children_ids()
            instance_id = general.find_editor_entity(prefab_file_name)
            assert instance_id not in original_parent_children_ids, \
                "Redo failed: Unexpectedly found prefab instance as a child of the original parent."
            assert instance_id in new_parent_children_ids, \
                "Redo failed: Failed to find instance as a child of the new parent."

        prefab_test_utils.open_base_tests_level()

        # Creates a new car entity at the root level
        car_entity = EditorEntity.create_editor_entity()
        car_prefab_entities = [car_entity]

        # Creates a prefab from the car entity
        _, car = Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME)

        # Creates another new wheel entity at the root level
        wheel_entity = EditorEntity.create_editor_entity()
        wheel_prefab_entities = [wheel_entity]

        # Creates another prefab from the wheel entity
        _, wheel = Prefab.create_prefab(wheel_prefab_entities, WHEEL_PREFAB_FILE_NAME)

        # Creates another new driver entity at the root level
        driver_entity = EditorEntity.create_editor_entity()
        driver_prefab_entities = [driver_entity]

        # Creates another prefab from the driver entity
        _, driver = Prefab.create_prefab(driver_prefab_entities, DRIVER_PREFAB_FILE_NAME)

        # Creates a few new entity hierarchies starting at the root level
        non_prefab_entity = EditorEntity.create_editor_entity("Non-Prefab Entity")
        non_prefab_parent_entity = EditorEntity.create_editor_entity("Non-Prefab Parent Entity")
        non_prefab_child_entity = EditorEntity.create_editor_entity("Non-Prefab Child Entity",
                                                                    non_prefab_parent_entity.id)

        # Ensure focus gets set on the prefab you want to parent under. This mirrors how users would do
        # reparenting in the editor.
        car.container_entity.focus_on_owning_prefab()

        # Reparents the wheel prefab instance to the container entity of the car prefab instance
        await reparent_with_undo_redo(WHEEL_PREFAB_FILE_NAME, wheel, car.container_entity.id)

        # Reparents the driver instance to the container entity of the now nested car/wheel prefab
        wheel.container_entity.focus_on_owning_prefab()
        await reparent_with_undo_redo(DRIVER_PREFAB_FILE_NAME, driver, wheel.container_entity.id)

        # Reparents the wheel prefab instance to the non-prefab entity at the root level
        non_prefab_entity.focus_on_owning_prefab()
        await reparent_with_undo_redo(WHEEL_PREFAB_FILE_NAME, wheel, non_prefab_entity.id)

        # Reparents the wheel prefab instance to the child entity of the non-prefab entity hierarchy
        non_prefab_child_entity.focus_on_owning_prefab()
        await reparent_with_undo_redo(WHEEL_PREFAB_FILE_NAME, wheel, non_prefab_child_entity.id)

    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ReparentPrefab_UnderPrefabAndEntityHierarchies)
