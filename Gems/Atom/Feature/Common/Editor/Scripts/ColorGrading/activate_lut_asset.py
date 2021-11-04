"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
This script is used by the Editor HDR Color Grading component to use a stored lut asset path
and pass it onto a Look Modification Component.

The HDR Color Grading component will be disabled as it is not compatible with the Look 
Modification Component.
"""
import azlmbr
import azlmbr.legacy.general as general

LOOK_MODIFICATION_LUT_PROPERTY_PATH = 'Controller|Configuration|Color Grading LUT'
LOOK_MODIFICATION_ENABLE_PROPERTY_PATH = 'Controller|Configuration|Enable look modification'
COLOR_GRADING_COMPONENT_ID = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["HDR Color Grading"], 0)
LOOK_MODIFICATION_COMPONENT_ID = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Look Modification"], 0)


def disable_hdr_color_grading_component(entity_id):
    componentOutcome = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'GetComponentOfType', entity_id, COLOR_GRADING_COMPONENT_ID[0])
    if(componentOutcome.IsSuccess()):
        azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'DisableComponents', [componentOutcome.GetValue()])

def add_look_modification_component(entity_id):
    componentOutcome = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'AddComponentsOfType', entity_id, LOOK_MODIFICATION_COMPONENT_ID)
    return componentOutcome.GetValue()[0]

def get_look_modification_component(entity_id):
    componentOutcome = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'GetComponentOfType', entity_id, LOOK_MODIFICATION_COMPONENT_ID[0])
    if componentOutcome.IsSuccess():
        return componentOutcome.GetValue()
    else:
        return None

def activate_look_modification_lut(look_modification_component, asset_relative_path):
    print(asset_relative_path)
    asset_id = azlmbr.asset.AssetCatalogRequestBus(
        azlmbr.bus.Broadcast, 
        'GetAssetIdByPath', 
        asset_relative_path, 
        azlmbr.math.Uuid(), 
        False
    )
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 
        'SetComponentProperty', 
        look_modification_component, 
        LOOK_MODIFICATION_LUT_PROPERTY_PATH, 
        asset_id
    )
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 
        'SetComponentProperty', 
        look_modification_component, 
        LOOK_MODIFICATION_ENABLE_PROPERTY_PATH, 
        True
    )
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        "EnableComponents",
        [look_modification_component]
    )

def activate_lut_asset(entity_id, asset_relative_path):
    disable_hdr_color_grading_component(entity_id)
    
    look_modification_component = get_look_modification_component(entity_id)
    if not look_modification_component:
        look_modification_component = add_look_modification_component(entity_id)
    
    general.idle_wait_frames(5)

    if look_modification_component:
        activate_look_modification_lut(look_modification_component, asset_relative_path)
    
    
if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument('--entityName', type=str, required=True, help='Entity ID to manage')
    parser.add_argument('--assetRelativePath', type=str, required=True, help='Lut asset relative path to activate')
    args=parser.parse_args()
    
    # Get the entity id
    searchFilter = azlmbr.entity.SearchFilter()
    searchFilter.names = [args.entityName]
    entityIdList = azlmbr.entity.SearchBus(azlmbr.bus.Broadcast, 'SearchEntities', searchFilter)

    for entityId in entityIdList:
        activate_lut_asset(entityId, args.assetRelativePath)
