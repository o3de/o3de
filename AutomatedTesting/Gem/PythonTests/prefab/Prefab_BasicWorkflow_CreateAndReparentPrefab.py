"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Prefab_BasicWorkflow_CreateAndReparentPrefab():

    CAR_PREFAB_FILE_NAME = 'car_prefab'
    WHEEL_PREFAB_FILE_NAME = 'wheel_prefab'

    import editor_python_test_tools.pyside_utils as pyside_utils

    @pyside_utils.wrap_async
    async def run_test():

        from editor_python_test_tools.editor_entity_utils import EditorEntity
        from prefab.Prefab import Prefab

        import prefab.Prefab_Test_Utils as prefab_test_utils

        prefab_test_utils.open_base_tests_level()

        # Creates a new Entity at the root level
        # Asserts if creation didn't succeed
        car_entity = EditorEntity.create_editor_entity()
        car_prefab_entities = [car_entity]

        # Checks for prefab creation passed or not 
        _, car = Prefab.create_prefab(
            car_prefab_entities, CAR_PREFAB_FILE_NAME)

        # Creates another new Entity at the root level
        wheel_entity = EditorEntity.create_editor_entity()
        wheel_prefab_entities = [wheel_entity]

        # Checks for wheel prefab creation passed or not 
        _, wheel = Prefab.create_prefab(
            wheel_prefab_entities, WHEEL_PREFAB_FILE_NAME)

        # Checks for prefab reparenting passed or not 
        await wheel.ui_reparent_prefab_instance(car.container_entity.id)

    run_test()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Prefab_BasicWorkflow_CreateAndReparentPrefab)
