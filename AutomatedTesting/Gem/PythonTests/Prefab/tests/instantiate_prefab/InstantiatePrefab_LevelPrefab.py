"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def InstantiatePrefab_LevelPrefab():
    """
    Test description:
    - Opens a simple level.
    - Instantiates a different level.prefab into the level, and validates its structure.
    - Validates Undo/Redo operations on the prefab instantiation.
    """

    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.prefab_utils import Prefab
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    prefab_test_utils.open_base_tests_level()

    # Find and instantiate an existing level.prefab
    test_level_prefab_path = os.path.join("Levels", "Prefab", "QuitOnSuccessfulSpawn", "QuitOnSuccessfulSpawn.prefab")
    test_level_prefab = Prefab.get_prefab(test_level_prefab_path)
    test_level_prefab_instance = test_level_prefab.instantiate()
    parent_entity = EditorEntity(test_level_prefab_instance.container_entity.get_parent_id())

    # Validate that the expected prefab is now present with the proper entity/component in our test level
    spawner_child_entity = test_level_prefab_instance.get_direct_child_entity_by_name("SC_Spawner")
    assert spawner_child_entity.has_component("Script Canvas"), \
        "Failed to find expected child entity with the proper component"

    # Validate Undo/Redo properly removes and re-instantiates the level.prefab
    general.undo()
    PrefabWaiter.wait_for_propagation()
    child_entities = parent_entity.get_children_ids()
    assert test_level_prefab_instance.container_entity.id not in child_entities, \
        "Undo Failed: Unexpectedly still found the prefab instance after Undo"

    general.redo()
    PrefabWaiter.wait_for_propagation()
    child_entities = parent_entity.get_children_ids()
    assert test_level_prefab_instance.container_entity.id in child_entities, \
        "Redo Failed: Failed to find prefab instance in the level after Redo"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(InstantiatePrefab_LevelPrefab)
