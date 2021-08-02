"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt:off
class Tests():
    create_new_entity = ("Entity: 'CreateNewEntity' passed", "Entity: 'CreateNewEntity' failed")
    create_prefab = ("Prefab: 'CreatePrefab' passed", "Prefab: 'CreatePrefab' failed")
    instantiate_prefab = ("Prefab: 'InstantiatePrefab' passed", "Prefab: 'InstantiatePrefab' failed")


# fmt:on

def PrefabLevel_BasicWorkflow():
    """
    Opens the level that contains 2 entities, "EmptyEntity" and "EntityWithPxCollider".
    This test makes sure that both entities exist after opening the level and that:
    - EmptyEntity is at Position: (10, 20, 30)
    - EntityWithPxCollider has a PhysXCollider component
    """

    import os
    import sys

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import editor_python_test_tools.hydra_editor_utils as hydra

    import azlmbr.entity as entity
    from azlmbr.entity import EntityId
    import azlmbr.editor as editor
    import azlmbr.bus as bus
    from azlmbr.math import Vector3

    EXPECTED_NEW_ENTITY_POS = Vector3(10.00, 20.0, 30.0)

    helper.init_idle()
    helper.open_level("Prefab", "PrefabLevel_BasicWorkflow")

# Create a new Entity at the root level
    newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    Report.result(Tests.create_new_entity, newEntityId.IsValid())

#Checks for prefab creation passed or not 
    prefab_worflow_result = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreatePrefab', [newEntityId], "path/to/new_entity_prefab", False)
    Report.result(Tests.create_prefab, prefab_worflow_result)

#Checks for prefab instantiation passed or not 
    prefab_worflow_result = editor.ToolsApplicationRequestBus(bus.Broadcast, 'InstantiatePrefab', "path/to/new_entity_prefab", EntityId(), EXPECTED_NEW_ENTITY_POS)
    Report.result(Tests.instantiate_prefab, prefab_worflow_result)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test (PrefabLevel_BasicWorkflow)
