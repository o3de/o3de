"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Test():

    CAR_PREFAB_FILE_NAME = 'car_prefab'
    WHEEL_PREFAB_FILE_NAME = 'wheel_prefab'

    import editor_python_test_tools.pyside_utils as pyside_utils

    @pyside_utils.wrap_async
    async def run_test():

        import prefab.Prefab_Operations as prefab
        import prefab.Prefab_Test_Utils as prefab_test_utils

        prefab_test_utils.open_base_tests_level()

        # Create a new Entity at the root level
        car_entity_id = prefab.create_entity()
        car_prefab_entity_ids = [car_entity_id]

        # Checks for car prefab creation passed or not 
        created_car_prefab_container_entity_id = prefab.create_prefab(
            car_prefab_entity_ids, CAR_PREFAB_FILE_NAME)

        # Create another new Entity at the root level
        wheel_entity_id = prefab.create_entity()
        wheel_prefab_entity_ids = [wheel_entity_id]

        # Checks for wheel prefab creation passed or not 
        created_wheel_prefab_container_entity_id = prefab.create_prefab(
            wheel_prefab_entity_ids, WHEEL_PREFAB_FILE_NAME)

        # Checks for prefab reparenting passed or not 
        await prefab.reparent_prefab(created_wheel_prefab_container_entity_id, created_car_prefab_container_entity_id)


    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Test)
