"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Prefab_BasicWorkflow_CreateAndDeletePrefab():

    CAR_PREFAB_FILE_NAME = 'car_prefab'
    CAR_PREFAB_INSTANCE_NAME = "car_1"

    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from prefab.Prefab import Prefab

    import prefab.Prefab_Test_Utils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Create a new Entity at the root level
    car_entity = Entity.create_editor_entity()
    car_prefab_entities = [car_entity]

    # Checks for prefab creation passed or not 
    car_prefab = Prefab.create_prefab(
        car_prefab_entities, CAR_PREFAB_FILE_NAME, prefab_instance_name=CAR_PREFAB_INSTANCE_NAME)

    # Checks for prefab deletion passed or not 
    car = car_prefab.instances[CAR_PREFAB_INSTANCE_NAME]
    Prefab.delete_prefabs([car])


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Prefab_BasicWorkflow_CreateAndDeletePrefab)
