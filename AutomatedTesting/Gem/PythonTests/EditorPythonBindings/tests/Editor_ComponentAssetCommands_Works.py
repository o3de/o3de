"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ComponentAssetCommands_Works(BaseClass):
    # Description: 
    # Tests a portion of the Component Property Get/Set, Python API while the Editor is running

    @staticmethod
    def GetSetCompareTest(component, path, assetId):
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.asset as asset

        # Test Get/Set (get old value, set new value, check that new value was set correctly)
        oldObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
        BaseClass.check_result(oldObj.IsSuccess(), "GetComponentProperty oldObj")
        oldValue = oldObj.GetValue()

        oldValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)

        editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, assetId)
        newObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)

        BaseClass.check_result(newObj.IsSuccess(), "GetComponentProperty newObj")
        newValue = newObj.GetValue()

        newValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, newValue)
        isOldNewValueSame = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)

        compareTest = not(newValue == oldValue) and oldValueCompared and newValueCompared and not isOldNewValueSame
        BaseClass.check_result(compareTest, "GetSetCompare Test")

        # Test Clear (set an invalid AssetId, check that the field was cleared correctly)
        editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, asset.AssetId())

        clearObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)

        BaseClass.check_result(clearObj.IsSuccess(), "clear object")
        clearValue = clearObj.GetValue()

        clearedValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, clearValue)
        isNewClearedValueSame = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, newValue)

        BaseClass.check_result((clearValue == asset.AssetId()) and clearedValueCompared and not isNewClearedValueSame, "GetSetCompare")

    @staticmethod
    def PteTest(pte, path, value):
        import azlmbr.asset as asset

        # Test Get/Set (get old value, set new value, check that new value was set correctly)
        oldObj = pte.get_value(path)
        BaseClass.check_result(oldObj.IsSuccess(), 'get_value')
        oldValue = oldObj.GetValue()

        oldValueCompared = pte.compare_value(path, oldValue)
        pte.set_value(path, value)

        newObj = pte.get_value(path)
        BaseClass.check_result(newObj.IsSuccess(), "get_value")
        newValue = newObj.GetValue()

        newValueCompared = pte.compare_value(path, newValue)
        isOldNewValueSame = pte.compare_value(path, oldValue)
        BaseClass.check_result(not(newValue == oldValue) and oldValueCompared and newValueCompared and not isOldNewValueSame, "compare_value")

        # Test Clear (set an invalid AssetId, check that the field was cleared correctly)
        pte.set_value(path, asset.AssetId())

        clearObj = pte.get_value(path)
        BaseClass.check_result(clearObj.IsSuccess(), "set_value")
        clearValue = clearObj.GetValue()

        clearedValueCompared = pte.compare_value(path, clearValue)
        isNewClearedValueSame = pte.compare_value(path, newValue)

        BaseClass.check_result((clearValue == asset.AssetId()) and clearedValueCompared and not isNewClearedValueSame, "compare_value")

    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.math as math
        import azlmbr.asset as asset

        # Create new Entity
        entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
        BaseClass.check_result(entityId, "New entity with no parent created")

        # Get Component Type for Mesh
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Mesh"], entity.EntityType().Game)

        componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)

        BaseClass.check_result(componentOutcome.IsSuccess(), "Mesh component added to entity")

        components = componentOutcome.GetValue()
        component = components[0]

        hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entityId, typeIdsList[0])
        BaseClass.check_result(hasComponent, "Entity has a Mesh component: SUCCESS")

        # Get the PTE from the Mesh Component
        pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', component)
        BaseClass.check_result(pteObj.IsSuccess(), "BuildComponentPropertyTreeEditor")
        pte = pteObj.GetValue()

        # Tests for the Asset<> case
        testAssetId = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', 'assets/objects/foliage/cedar.fbx.azmodel', math.Uuid(), False)
        Editor_ComponentAssetCommands_Works.GetSetCompareTest(component, "Controller|Configuration|Model Asset", testAssetId)
        Editor_ComponentAssetCommands_Works.PteTest(pte, "Controller|Configuration|Model Asset", testAssetId)

if __name__ == "__main__":
    tester = Editor_ComponentAssetCommands_Works()
    tester.test_case(tester.test)
