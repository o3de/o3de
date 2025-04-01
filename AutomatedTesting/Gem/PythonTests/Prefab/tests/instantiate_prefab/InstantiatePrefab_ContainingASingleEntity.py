"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def InstantiatePrefab_ContainingASingleEntity():

    from azlmbr.math import Vector3
    import azlmbr.legacy.general as general

    EXISTING_TEST_PREFAB_FILE_NAME = "Gem/PythonTests/Prefab/data/Test.prefab"
    INSTANTIATED_TEST_PREFAB_POSITION = Vector3(10.00, 20.0, 30.0)
    EXPECTED_TEST_PREFAB_CHILDREN_COUNT = 1

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Instantiates a new car prefab instance
    test_prefab = Prefab.get_prefab(EXISTING_TEST_PREFAB_FILE_NAME)
    test_instance = test_prefab.instantiate( 
        prefab_position=INSTANTIATED_TEST_PREFAB_POSITION)

    # Get parent entity and container id for verifying successful Undo/Redo operations
    instance_parent_id = EditorEntity(test_instance.container_entity.get_parent_id())
    instance = test_instance.container_entity

    prefab_test_utils.check_entity_children_count(
        test_instance.container_entity.id, 
        EXPECTED_TEST_PREFAB_CHILDREN_COUNT)

    # Undo the instantiation
    general.undo()
    PrefabWaiter.wait_for_propagation()
    child_ids = instance_parent_id.get_children_ids()
    assert instance.id not in child_ids, "Undo Failed: Instance was still found after undo."

    # Redo the instantiation
    general.redo()
    PrefabWaiter.wait_for_propagation()
    child_ids = instance_parent_id.get_children_ids()
    instance_children = instance.get_children()
    assert instance.id in child_ids, "Redo Failed: Instance was not found after redo"
    assert len(instance_children) == EXPECTED_TEST_PREFAB_CHILDREN_COUNT, \
        "Redo Failed: Did not find expected child entities in the prefab instance"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(InstantiatePrefab_ContainingASingleEntity)
