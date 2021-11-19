"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt:off
class Tests():
    find_empty_entity  = ("Entity: 'EmptyEntity' found",                         "Entity: 'EmptyEntity' *not* found in level")
    empty_entity_pos   = ("'EmptyEntity' position is at the expected position",  "'EmptyEntity' position is *not* at the expected position")
    find_pxentity      = ("Entity: 'EntityWithPxCollider' found",                "Entity: 'EntityWithPxCollider' *not* found in level")
    pxentity_component = ("Entity: 'EntityWithPxCollider' has a Physx Collider", "Entity: 'EntityWithPxCollider' does *not* have a Physx Collider")

# fmt:on

def PrefabLevel_OpensLevelWithEntities():
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
    import azlmbr.bus as bus
    from azlmbr.math import Vector3

    EXPECTED_EMPTY_ENTITY_POS = Vector3(10.00, 20.0, 30.0)

    helper.init_idle()
    helper.open_level("Prefab", "PrefabLevel_OpensLevelWithEntities")

    def find_entity(entity_name):
        searchFilter = entity.SearchFilter()
        searchFilter.names = [entity_name]
        entityIds = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
        if entityIds[0].IsValid():
            return entityIds[0]
        return None
    
    # Checks for an entity called "EmptyEntity"
    helper.wait_for_condition(lambda: find_entity("EmptyEntity").IsValid(), 5.0)
    empty_entity_id = find_entity("EmptyEntity")
    Report.result(Tests.find_empty_entity, empty_entity_id.IsValid())

    # Checks if the EmptyEntity is in the correct position and if it fails, it will provide the expected postion and the actual postion of the entity in the Editor log
    empty_entity_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", empty_entity_id)
    is_at_position = empty_entity_pos.IsClose(EXPECTED_EMPTY_ENTITY_POS)
    Report.result(Tests.empty_entity_pos, is_at_position)
    if not is_at_position:
        Report.info(f'Expected position: {EXPECTED_EMPTY_ENTITY_POS.ToString()}, actual position: {empty_entity_pos.ToString()}')

    # Checks for an entity called "EntityWithPxCollider" and if it has the PhysX Collider component
    pxentity = find_entity("EntityWithPxCollider")
    Report.result(Tests.find_pxentity, pxentity.IsValid())

    pxcollider_id = hydra.get_component_type_id("PhysX Collider")
    hasComponent = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'HasComponentOfType', pxentity, pxcollider_id)
    Report.result(Tests.pxentity_component, hasComponent)

if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabLevel_OpensLevelWithEntities)
