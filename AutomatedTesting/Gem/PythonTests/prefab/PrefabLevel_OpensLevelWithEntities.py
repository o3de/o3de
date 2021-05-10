"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# fmt:off
class Tests ():
    find_empty_entity = ("Entity: 'EmptyEntity' found", "Entity: 'EmptyEntity' *not* found in level")
    empty_entity_pos = (
    "'EmptyEntity' position is at the expected position", "'EmptyEntity' position is *not* at the expected position")
    find_pxentity = ("Entity: 'EntityWithPxCollider' found", "Entity: 'EntityWithPxCollider' *not* found in level")
    pxentity_component = (
    "Entity: 'EntityWithPxCollider' has a Physx Collider", "Entity: 'EntityWithPxCollider' has *not* a Physx Collider")

# fmt:on

def PrefabLevel_OpensLevelWithEntities ():
    """
    Opens the level that contains 2 entities, "EmptyEntity" and "EntityWithPxCollider".
    This test makes sure that both entities exist after openning the level and that:
    - EmptyEntity is at Position: (10, 20, 30)
    - EntityWithPxCollider has a PhysXCollider component
    """

    import os
    import sys

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import editor_python_test_tools.hydra_editor_utils as hydra

    import azlmbr.legacy.general as general
    import azlmbr.bus
    from azlmbr.math import Vector3

    EXPECTED_EMPTY_ENTITY_POS = Vector3 (10.00, 20.0, 30.0)

    helper.init_idle ()
    helper.open_level ("prefab", "PrefabLevel_OpensLevelWithEntities")

    class EmptyEntity ():
        value = None

    def find_empty_entity ():
        EmptyEntity.value = general.find_editor_entity ("EmptyEntity")
        return EmptyEntity.value.IsValid ()

    helper.wait_for_condition (find_empty_entity, 5.0)
    Report.result (Tests.find_empty_entity, EmptyEntity.value.IsValid ())

    empty_entity_pos = azlmbr.components.TransformBus (azlmbr.bus.Event, "GetWorldTranslation", EmptyEntity.value)
    is_at_position = empty_entity_pos.IsClose (EXPECTED_EMPTY_ENTITY_POS)
    Report.result (Tests.empty_entity_pos, is_at_position)
    if not is_at_position:
        Report.info (f'Expected position: {EXPECTED_EMPTY_ENTITY_POS.ToString ()}, actual position: {empty_entity_pos.ToString ()}')

    pxentity = general.find_editor_entity ("EntityWithPxCollider")
    Report.result (Tests.find_pxentity, pxentity.IsValid ())

    pxcollider_id = hydra.get_component_type_id ("PhysX Collider")
    hasComponent = azlmbr.editor.EditorComponentAPIBus (azlmbr.bus.Broadcast, 'HasComponentOfType', pxentity,
                                                        pxcollider_id)
    Report.result (Tests.pxentity_component, hasComponent)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test (PrefabLevel_OpensLevelWithEntities)
    PrefabLevel_OpensLevelWithEntities ()