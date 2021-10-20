"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PrefabBasicWorkflow_CreateReparentAndDetachPrefab():

    CAR_PREFAB_FILE_NAME = 'car_prefab'
    WHEEL_PREFAB_FILE_NAME = 'wheel_prefab'

    import editor_python_test_tools.pyside_utils as pyside_utils

    @pyside_utils.wrap_async
    async def run_test():

        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from editor_python_test_tools.prefab_utils import Prefab

        import PrefabTestUtils as prefab_test_utils

        prefab_test_utils.open_base_tests_level()

        # Creates a new Entity at the root level
        # Asserts if creation didn't succeed
        car_entity = EditorEntity.create_editor_entity()
        car_prefab_entities = [car_entity]

        # Asserts if prefab creation doesn't succeeds 
        _, car = Prefab.create_prefab(
            car_prefab_entities, CAR_PREFAB_FILE_NAME)

        # Creates another new Entity at the root level
        # Asserts if creation didn't succeed
        wheel_entity = EditorEntity.create_editor_entity()
        wheel_prefab_entities = [wheel_entity]

        # Asserts if prefab creation doesn't succeeds
        _, wheel = Prefab.create_prefab(
            wheel_prefab_entities, WHEEL_PREFAB_FILE_NAME)

        # Asserts if prefab reparenting doesn't succeeds
        await wheel.ui_reparent_prefab_instance(car.container_entity.id)

        # Asserts if prefab detachment doesn't succeeds
        Prefab.detach_prefab(wheel)

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabBasicWorkflow_CreateReparentAndDetachPrefab)
