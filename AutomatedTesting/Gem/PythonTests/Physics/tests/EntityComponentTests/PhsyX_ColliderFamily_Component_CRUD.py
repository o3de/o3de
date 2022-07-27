"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests():
    create_collider_entity = ("PhysX Collider Entity Created", "PhysX Collider Entity failed to respond after creation")
    create_shape_collider_entity = ("PhysX Shape Collider Entity Created", "PhysX Shape Collider Entity failed to respond after creation")


def PhysX_ColliderFamily_Component_CRUD():
    """
    Summary:
    Test Does Something

    Expected Behavior:
    Some Behavior

    PhysX Collider Family Components:
     - PhysX Collider
     - PhysX Shape Collider

    Test Steps:
     1) Open Base Level
     2) Add an Entity for each Collider Family Component
     3) Add each Collider Family Component to its own entity
     4) Manipulate each modifiable field and validate they are in expected state
     5) Validate Fields are in expected state
     6) Delete components and validate its removal

    :return: None
    """

    # Helper file Imports

    import azlmbr.math as math
    import azlmbr.entity as EntityId
    import azlmbr.editor as editor
    import azlmbr.bus as bus

    import editor_python_test_tools.hydra_editor_utils as hydra

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    from editor_python_test_tools.asset_utils import Asset
    from ...utils.physics_constants import (WAIT_TIME_3)


    def create_entity(entity_name, components_to_add):
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        entity = hydra.Entity(entity_name, entity_id)
        if entity_id.IsValid():
            print(f"{entity_name} entity Created")
        entity.components = []
        for component in components_to_add:
            entity.components.append(hydra.add_component(component, entity_id))
        return entity

    # 1) Load the level
    hydra.open_base_level()

    # 2) Adding Entities & Verifying they were created
    physx_collider = EditorEntity.create_editor_entity("PhsyX_Collider")
    shape_collider = EditorEntity.create_editor_entity("Shape_Collider")

    collider_entity_found = helper.wait_for_condition(physx_collider.id.IsValid(), WAIT_TIME_3)
    shape_collider_entity_found = helper.wait_for_condition(shape_collider.id.IsValid(), WAIT_TIME_3)

    Report.result(Tests.create_collider_entity, collider_entity_found)
    Report.result(Tests.create_shape_collider_entity, shape_collider_entity_found)

    # 3) Add Components to Entities

