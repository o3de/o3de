"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def CreatePrefab_CreationFailsWithDifferentRootEntities():
    """
    Test description:
    - Creates a single entity and a nested entity hierarchy at the root level
    - Selects the single entity and one of the nested entities
    - Attempts to create a prefab from the selected entities
    Test is successful if prefab creation fails and error messaging matches expected
    """

    from pathlib import Path

    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    TEST_PREFAB_FILE_NAME = Path(__file__).stem + '_prefab'
    SINGLE_ENTITY_NAME = "SingleEntity"
    NESTED_ENTITY_PREFIX = "NestedEntity_"
    NESTED_ENTITY_POS = math.Vector3(0.0, 0.0, 0.0)
    EXPECTED_ERR_STR = "Prefab operation 'CreatePrefab' failed. Error: Failed to create a prefab: Provided entities do " \
                       "not share a common root."

    prefab_test_utils.open_base_tests_level()

    # Creates a new Entity at the root level
    test_entity = EditorEntity.create_editor_entity(SINGLE_ENTITY_NAME)
    assert test_entity.exists(), f"Failed to create entity with name {SINGLE_ENTITY_NAME}"

    # Creates a new Entity hierarchy at the root level
    nested_entity_root = prefab_test_utils.create_linear_nested_entities(NESTED_ENTITY_PREFIX, 3, NESTED_ENTITY_POS)
    prefab_test_utils.validate_linear_nested_entities(nested_entity_root, 3, NESTED_ENTITY_POS)
    child_test_entity = EditorEntity.find_editor_entity("NestedEntity_2")

    # Create a prefab from the single entity and a child entity of the nested entity hierarchy
    test_prefab_instance = None
    try:
        _, test_prefab_instance = Prefab.create_prefab([test_entity, child_test_entity], TEST_PREFAB_FILE_NAME)
    except AssertionError as error:
        prefab_creation_error = str(error)
        Report.info(prefab_creation_error)

    # Validate that the error message from create_prefab matches expected message
    assert not test_prefab_instance, "Prefab creation unexpectedly succeeded"
    assert prefab_creation_error == EXPECTED_ERR_STR, f"Expected Error: {EXPECTED_ERR_STR}, Found Error: " \
                                                      f"{prefab_creation_error}"


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(CreatePrefab_CreationFailsWithDifferentRootEntities)
