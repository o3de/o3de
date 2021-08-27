"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from azlmbr.math import Vector3
from editor_python_test_tools.editor_test_helper import EditorTestHelper

import editor_python_test_tools.pyside_utils as pyside_utils
import prefab.Prefab_Operations as prefab
import prefab.Prefab_Test_Utils as prefab_test_utils


class PrefabBasicWorkflowInstantiatePrefabTest(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="Prefab_BasicWorkflow_InstantiatePrefab", args=["level"])

    EXISTING_TEST_PREFAB_FILE_NAME = "Test"
    INSTANTIATED_TEST_PREFAB_POSITION = Vector3(10.00, 20.0, 30.0)
    EXPECTED_TEST_PREFAB_CHILDREN_COUNT = 1

    @pyside_utils.wrap_async
    async def run_test(self):
        self.test_success = self.open_level(self.args["level"])

        # Checks for prefab instantiation passed or not 
        instantiated_car_container_entity_id = prefab.instantiate_prefab(
            PrefabBasicWorkflowInstantiatePrefabTest.EXISTING_TEST_PREFAB_FILE_NAME, 
            expected_prefab_position=PrefabBasicWorkflowInstantiatePrefabTest.INSTANTIATED_TEST_PREFAB_POSITION)
        
        prefab_test_utils.check_entity_children_count(instantiated_car_container_entity_id, EXPECTED_TEST_PREFAB_CHILDREN_COUNT)

        
test = PrefabBasicWorkflowInstantiatePrefabTest()
test.run()
