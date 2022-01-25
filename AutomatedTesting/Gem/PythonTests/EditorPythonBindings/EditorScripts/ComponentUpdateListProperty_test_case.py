"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


import os
import sys

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.projectroot, 'Gem', 'PythonTests'))
from automatedtesting_shared.editor_test_helper import EditorTestHelper


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

class TestComponentUpdateListProperty(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="ComponentUpdateListProperty", args=["level"])

    def run_test(self):
        """
        Test configuring an asset descriptor list.
        """

        # Create a new level
        self.test_success = self.create_level(
            self.args["level"],
        )

        newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

        editorDescriptorListComponentId = math.Uuid_CreateString('{3AF9BE58-6D2D-44FB-AB4D-CA1182F6C78F}', 0)
        descListComponent = add_component_with_uuid(newEntityId, editorDescriptorListComponentId)

        primitiveCubeId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_cube.cgf', math.Uuid(), False)
        primitiveSphereId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_sphere.cgf', math.Uuid(), False)
        primitiveCapsuleId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_capsule.cgf', math.Uuid(), False)
        primitivePlaneId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'objects/default/primitive_plane.cgf', math.Uuid(), False)

        # expand the list of veg descriptors to 4 elements
        descList = get_component_property(descListComponent, 'Configuration|Embedded Assets')
        descElem = descList[0]
        descList.append(descElem)
        descList.append(descElem)
        descList.append(descElem)

        self.test_success = set_component_property(descListComponent, 'Configuration|Embedded Assets', descList) and self.test_success
        print('Set EditorDescriptorListComponent Embedded Assets as List')
        self.test_success = set_component_property(descListComponent, 'Configuration|Embedded Assets|[0]|Instance|Mesh Asset', primitiveCubeId) and self.test_success
        print('Set EditorDescriptorListComponent Embedded Assets 0 Descriptor Mesh Asset ID')
        self.test_success = set_component_property(descListComponent, 'Configuration|Embedded Assets|[1]|Instance|Mesh Asset', primitiveSphereId) and self.test_success
        print('Set EditorDescriptorListComponent Embedded Assets 1 Descriptor Mesh Asset ID')
        self.test_success = set_component_property(descListComponent, 'Configuration|Embedded Assets|[2]|Instance|Mesh Asset', primitiveCapsuleId) and self.test_success
        print('Set EditorDescriptorListComponent Embedded Assets 2 Descriptor Mesh Asset ID')
        self.test_success = set_component_property(descListComponent, 'Configuration|Embedded Assets|[3]|Instance|Mesh Asset', primitivePlaneId) and self.test_success
        print('Set EditorDescriptorListComponent Embedded Assets 3 Descriptor Mesh Asset ID')

test = TestComponentUpdateListProperty()
test.run()
