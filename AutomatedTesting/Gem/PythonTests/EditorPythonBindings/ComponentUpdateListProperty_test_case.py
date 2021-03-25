"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import azlmbr.object as object
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
import azlmbr.entity as entity
import azlmbr.math as math
import azlmbr.asset as asset


def add_component_with_uuid(entityId, typeId):
    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, [typeId])
    if (componentOutcome.IsSuccess()):
        return componentOutcome.GetValue()[0]


def set_component_property(component, path, value):
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
    return outcome.IsSuccess()


def get_component_property(component, path):
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
    if (outcome.IsSuccess()):
        return outcome.GetValue()
    return None


try:
    # Open a level
    print ('Test started')
    general.open_level_no_prompt('auto_test')

    newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

    editorDescriptorListComponentId = math.Uuid_CreateString('{3AF9BE58-6D2D-44FB-AB4D-CA1182F6C78F}', 0)
    descListComponent = add_component_with_uuid(newEntityId, editorDescriptorListComponentId)
    print ('descListComponent added')

    primitiveCubeId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_cube.cgf', math.Uuid(), False)
    primitiveSphereId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_sphere.cgf', math.Uuid(), False)
    primitiveCapsuleId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_capsule.cgf', math.Uuid(), False)
    primitivePlaneId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_plane.cgf', math.Uuid(), False)
    print ('fetched asset ids')

    # expand the list of veg descriptors to 4 elements
    descList = get_component_property(descListComponent, 'Configuration|Embedded Assets')
    print('Got the veg descriptor list')
    descElem = descList[0]
    descList.append(descElem)
    descList.append(descElem)
    descList.append(descElem)

    set_component_property(descListComponent, 'Configuration|Embedded Assets', descList)
    print('Set EditorDescriptorListComponent Embedded Assets as List')
    set_component_property(descListComponent, 'Configuration|Embedded Assets|[0]|Instance|Mesh Asset', primitiveCubeId)
    print('Set EditorDescriptorListComponent Embedded Assets 0 Descriptor Mesh Asset ID')
    set_component_property(descListComponent, 'Configuration|Embedded Assets|[1]|Instance|Mesh Asset', primitiveSphereId)
    print('Set EditorDescriptorListComponent Embedded Assets 1 Descriptor Mesh Asset ID')
    set_component_property(descListComponent, 'Configuration|Embedded Assets|[2]|Instance|Mesh Asset', primitiveCapsuleId)
    print('Set EditorDescriptorListComponent Embedded Assets 2 Descriptor Mesh Asset ID')
    set_component_property(descListComponent, 'Configuration|Embedded Assets|[3]|Instance|Mesh Asset', primitivePlaneId)
    print('Set EditorDescriptorListComponent Embedded Assets 3 Descriptor Mesh Asset ID')
except:
    print ('Test failed.')
finally:
    print ('Test done.')
    general.exit_no_prompt()
