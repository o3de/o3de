"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from editor_python_test_tools.editor_test_helper import EditorTestHelper

import editor_python_test_tools.pyside_utils as pyside_utils
import prefab.Prefab_Operations as prefab


class PrefabBasicWorkflowCreateAndDeletePrefabTest(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="Prefab_BasicWorkflow_CreateAndDeletePrefab", args=["level"])


    @pyside_utils.wrap_async
    async def run_test(self):
        self.test_success = self.open_level(self.args["level"])

        # Create a new Entity at the root level
        car_entity_id = prefab.create_entity(entity_name='CAR_entity')
        car_prefab_entity_ids = [car_entity_id]

        # Checks for prefab creation passed or not 
        created_car_prefab_container_entity_id = prefab.create_prefab(car_prefab_entity_ids, 'CAR_prefab')

        # Checks for prefab deletion passed or not 
        prefab.delete_prefabs([created_car_prefab_container_entity_id])


test = PrefabBasicWorkflowCreateAndDeletePrefabTest()
test.run()
