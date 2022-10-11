"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def DetachPrefab_UnderAnotherPrefab():

    from pathlib import Path
    CAR_PREFAB_FILE_NAME = Path(__file__).stem + 'car_prefab'
    WHEEL_PREFAB_FILE_NAME = Path(__file__).stem + 'wheel_prefab'

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():

        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.legacy.general as general

        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from editor_python_test_tools.prefab_utils import Prefab
        from editor_python_test_tools.wait_utils import PrefabWaiter
        import Prefab.tests.PrefabTestUtils as prefab_test_utils

        def validate_entity_hierarchy(condition_checked):
            search_filter = entity.SearchFilter()
            search_filter.names = [WHEEL_PREFAB_FILE_NAME]
            wheel_prefab_root = EditorEntity(entity.SearchBus(bus.Broadcast, "SearchEntities", search_filter)[0])
            search_filter.names = ["Wheel"]
            wheel_entity = EditorEntity(entity.SearchBus(bus.Broadcast, "SearchEntities", search_filter)[0])
            car_prefab_children = car.get_direct_child_entities()
            car_prefab_children_ids = [child.id for child in car_prefab_children]
            wheel_prefab_root_children = wheel_prefab_root.get_children()
            wheel_prefab_root_children_ids = [child.id for child in wheel_prefab_root_children]
            assert wheel_entity.id in wheel_prefab_root_children_ids and \
                   wheel_prefab_root.id in car_prefab_children_ids, \
                   f"Entity hierarchy does not match expectations after {condition_checked}"

        prefab_test_utils.open_base_tests_level()

        # Creates a new car entity at the root level
        car_entity = EditorEntity.create_editor_entity("Car")
        car_prefab_entities = [car_entity]

        # Creates a prefab from the car entity
        _, car = Prefab.create_prefab(
            car_prefab_entities, CAR_PREFAB_FILE_NAME)

        # Creates another new wheel entity at the root level
        wheel_entity = EditorEntity.create_editor_entity("Wheel")
        wheel_prefab_entities = [wheel_entity]

        # Creates another prefab from the wheel entity
        _, wheel = Prefab.create_prefab(
            wheel_prefab_entities, WHEEL_PREFAB_FILE_NAME)

        # Ensure focus gets set on the prefab you want to parent under. This mirrors how users would do
        # reparenting in the editor.
        car.container_entity.focus_on_owning_prefab()

        # Reparents the wheel prefab instance to the container entity of the car prefab instance
        await wheel.ui_reparent_prefab_instance(car.container_entity.id)

        # Detaches the wheel prefab instance
        Prefab.detach_prefab(wheel)

        # Test undo/redo on prefab detach
        general.undo()
        PrefabWaiter.wait_for_propagation()
        is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", wheel.container_entity.id,
                                                 azlmbr.globals.property.EditorPrefabComponentTypeId)
        assert is_prefab, "Undo operation failed. Entity is not recognized as a prefab."

        # Verify entity hierarchy is the same after undoing prefab detach
        validate_entity_hierarchy("Undo")

        general.redo()
        PrefabWaiter.wait_for_propagation()
        is_prefab = editor.EditorComponentAPIBus(bus.Broadcast, "HasComponentOfType", wheel.container_entity.id,
                                                 azlmbr.globals.property.EditorPrefabComponentTypeId)
        assert not is_prefab, "Redo operation failed. Entity is still recognized as a prefab."

        # Verify entity hierarchy is unchanged after redoing the prefab detach
        validate_entity_hierarchy("Redo")

    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(DetachPrefab_UnderAnotherPrefab)
