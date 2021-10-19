"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PrefabBasicWorkflow_CreatePrefab():

    CAR_PREFAB_FILE_NAME = 'car_prefab'

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.prefab_utils import Prefab

    import PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    # Asserts if creation didn't succeed
    car_entity = EditorEntity.create_editor_entity()
    car_prefab_entities = [car_entity]

    # Checks for prefab creation passed or not
    Prefab.create_prefab(car_prefab_entities, CAR_PREFAB_FILE_NAME)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabBasicWorkflow_CreatePrefab)
