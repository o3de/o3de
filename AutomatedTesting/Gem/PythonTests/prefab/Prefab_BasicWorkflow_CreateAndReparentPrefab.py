"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from editor_python_test_tools.editor_test_helper import EditorTestHelper

import editor_python_test_tools.pyside_utils as pyside_utils
import prefab.Prefab_Operations as prefab


class PrefabBasicWorkflowCreateAndReparentPrefabTest(EditorTestHelper):

    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="Prefab_BasicWorkflow_CreateAndReparentPrefab", args=["level"])


    @pyside_utils.wrap_async
    async def run_test(self):
        self.test_success = self.open_level(self.args["level"])

        # Create a new Entity at the root level
        car_entity_id = prefab.create_entity(entity_name='CAR_entity')
        car_prefab_entity_ids = [car_entity_id]

        # Checks for car prefab creation passed or not 
        created_car_prefab_container_entity_id = prefab.create_prefab(car_prefab_entity_ids, 'CAR_prefab')

        # Create another new Entity at the root level
        wheel_entity_id = prefab.create_entity(entity_name='WHEEL_entity')
        wheel_prefab_entity_ids = [wheel_entity_id]

        # Checks for wheel prefab creation passed or not 
        created_wheel_prefab_container_entity_id = prefab.create_prefab(wheel_prefab_entity_ids, 'WHEEL_prefab')

        # Checks for prefab reparenting passed or not 
        await prefab.reparent_prefab(created_wheel_prefab_container_entity_id, created_car_prefab_container_entity_id)


test = PrefabBasicWorkflowCreateAndReparentPrefabTest()
test.run()
