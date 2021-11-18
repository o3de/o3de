"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def InstantiatePrefab_ContainingASingleEntity():

    from azlmbr.math import Vector3

    EXISTING_TEST_PREFAB_FILE_NAME = "Gem/PythonTests/Prefab/data/Test.prefab"
    INSTANTIATED_TEST_PREFAB_POSITION = Vector3(10.00, 20.0, 30.0)
    EXPECTED_TEST_PREFAB_CHILDREN_COUNT = 1

    from editor_python_test_tools.prefab_utils import Prefab

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Instantiates a new car prefab instance
    test_prefab = Prefab.get_prefab(EXISTING_TEST_PREFAB_FILE_NAME)
    test_instance = test_prefab.instantiate( 
        prefab_position=INSTANTIATED_TEST_PREFAB_POSITION)
    
    prefab_test_utils.check_entity_children_count(
        test_instance.container_entity.id, 
        EXPECTED_TEST_PREFAB_CHILDREN_COUNT)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(InstantiatePrefab_ContainingASingleEntity)
