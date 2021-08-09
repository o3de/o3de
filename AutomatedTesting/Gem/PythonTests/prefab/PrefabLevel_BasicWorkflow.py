"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt:off
class Tests():
    create_new_entity =  ("Entity: 'CreateNewEntity' passed",                          "Entity: 'CreateNewEntity' failed")
    create_prefab =      ("Prefab: 'CreatePrefab' passed",                             "Prefab: 'CreatePrefab' failed")
    instantiate_prefab = ("Prefab: 'InstantiatePrefab' passed",                        "Prefab: 'InstantiatePrefab' failed")
    new_prefab_pos =     ("Prefab: new prefab's position is at the expected position", "Prefab: new prefab's position is *not* at the expected position")

# fmt:on

def PrefabLevel_BasicWorkflow():
    """
    This test will help verify if the following functions related to Prefab work as expected: 
    - CreatePrefab
    - InstantiatePrefab
    """

    import os
    import sys

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import editor_python_test_tools.hydra_editor_utils as hydra

    import azlmbr.bus as bus
    import azlmbr.entity as entity
    from azlmbr.entity import EntityId
    import azlmbr.editor as editor
    import azlmbr.prefab as prefab
    from azlmbr.math import Vector3

    EXPECTED_NEW_PREFAB_POS = Vector3(10.00, 20.0, 30.0)

    helper.init_idle()
    helper.open_level("Prefab", "PrefabLevel_BasicWorkflow")

# Create a new Entity at the root level
    new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    Report.result(Tests.create_new_entity, new_entity_id.IsValid())

# Checks for prefab creation passed or not 
    create_prefab_result = prefab.PrefabPublicRequestBus(bus.Broadcast, 'CreatePrefabInMemory', [new_entity_id], "path/to/new_entity_prefab")
    Report.result(Tests.create_prefab, create_prefab_result)

# Checks for prefab instantiation passed or not 
    container_entity_id = prefab.PrefabPublicRequestBus(bus.Broadcast, 'InstantiatePrefab', "path/to/new_entity_prefab", EntityId(), EXPECTED_NEW_PREFAB_POS)
    Report.result(Tests.instantiate_prefab, container_entity_id.IsValid())

# Checks if the new prefab is at the correct position and if it fails, it will provide the expected postion and the actual postion of the entity in the Editor log
    new_prefab_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", container_entity_id)
    is_at_position = new_prefab_pos.IsClose(EXPECTED_NEW_PREFAB_POS)
    Report.result(Tests.new_prefab_pos, is_at_position)
    if not is_at_position:
        Report.info(f'Expected position: {EXPECTED_NEW_PREFAB_POS.ToString()}, actual position: {new_prefab_pos.ToString()}')
    

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabLevel_BasicWorkflow)
