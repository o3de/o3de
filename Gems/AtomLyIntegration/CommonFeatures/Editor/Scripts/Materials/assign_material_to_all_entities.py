# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
    Apply a hard-coded material to all meshes in the level
"""

import os

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.editor as editor
import azlmbr.render as render

###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    
    # Get the material we want to assign
    material_asset_path = os.path.join("materials", "special", "debugvertexstreams.azmaterial")
    material_asset_id = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", material_asset_path, math.Uuid(), False)
    general.log(f"Assigning {material_asset_path} to all mesh and actor components in the level.")

    type_ids_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Mesh", "Actor", "Material"], 0)
    
    mesh_component_type_id = type_ids_list[0]
    actor_component_type_id = type_ids_list[1]
    material_component_type_id = type_ids_list[2]
    
    # For each entity
    search_filter = entity.SearchFilter()
    all_entities = entity.SearchBus(bus.Broadcast, "SearchEntities", search_filter)
    
    for current_entity_id in all_entities:
        # If entity has Mesh or Actor component
        has_mesh_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', current_entity_id, mesh_component_type_id)
        has_actor_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', current_entity_id, actor_component_type_id)
        if has_mesh_component or has_actor_component:
            # If entity does not have Material component
            has_material_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', current_entity_id, material_component_type_id)
            if not has_material_component:
                # Add material component
                editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', current_entity_id, [material_component_type_id])
            
            # Clear all material overrides
            render.MaterialComponentRequestBus(bus.Event, 'ClearAllMaterialOverrides', current_entity_id)

            # Set the default material to the one we want, which will apply it to every slot since we've cleared all overrides
            render.MaterialComponentRequestBus(bus.Event, 'SetDefaultMaterialOverride', current_entity_id, material_asset_id)

