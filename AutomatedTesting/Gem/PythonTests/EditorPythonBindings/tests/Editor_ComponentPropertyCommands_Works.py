"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ComponentPropertyCommands_Works(BaseClass):
    # Description: 
    # Tests a portion of the Component Property Get/Set Python API while the Editor is running

    @staticmethod
    def GetSetCompareTest(component, path, value):
        import azlmbr.bus as bus
        import azlmbr.editor as editor

        oldObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
        BaseClass.check_result(oldObj.IsSuccess(), 'oldObj.IsSuccess()')
        oldValue = oldObj.GetValue()

        oldValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)
        editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
        newObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
        BaseClass.check_result(newObj.IsSuccess(), 'newObj.IsSuccess()')
        newValue = newObj.GetValue()

        newValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, newValue)
        isOldNewValueSame = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)
        BaseClass.check_result(not(newValue == oldValue and oldValueCompared and newValueCompared and not isOldNewValueSame), "GetSetCompareTest " + path)

    @staticmethod
    def PteTest(pte, path, value):
        oldObj = pte.get_value(path)
        BaseClass.check_result(oldObj.IsSuccess(), 'oldObj.IsSuccess()')
        oldValue = oldObj.GetValue()
        oldValueCompared = pte.compare_value(path, oldValue)

        pte.set_value(path, value)
        newObj = pte.get_value(path)
        BaseClass.check_result(newObj.IsSuccess(), 'newObj.IsSuccess()')
        newValue = newObj.GetValue()

        newValueCompared = pte.compare_value(path, newValue)
        isOldNewValueSame = pte.compare_value(path, oldValue)
        BaseClass.check_result(not(newValue == oldValue and oldValueCompared and newValueCompared and not isOldNewValueSame), "PteTest " + path)
    
    @staticmethod
    def test():
        import azlmbr.legacy.general
        import azlmbr.prefab
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.math as math
        check_result = BaseClass.check_result
        GetSetCompareTest = Editor_ComponentPropertyCommands_Works.GetSetCompareTest
        PteTest = Editor_ComponentPropertyCommands_Works.PteTest

        # Create new Entity
        entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
        check_result(entityId, "New entity with no parent created")

        # Get Component Type for Quad Shape
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Quad Shape"], entity.EntityType().Game)

        componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
        check_result(componentOutcome.IsSuccess(), f"Quad Shape component {typeIdsList} added to entity")

        components = componentOutcome.GetValue()
        component = components[0]

        hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entityId, typeIdsList[0])
        check_result(hasComponent, "Entity has an Quad Shape component")

        # Test BuildComponentPropertyList
        paths = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', component)    
        check_result(len(paths) > 1, f"BuildComponentPropertyList {len(paths)}")

        # Tests for GetComponentProperty/SetComponentProperty
        path_color = 'Shape Color'
        path_visible = 'Visible'
        path_quad_width = 'Quad Shape|Quad Configuration|Width'

        GetSetCompareTest(component, path_visible, False)
        GetSetCompareTest(component, path_quad_width, 42.0)

        color = math.Color()
        color.r = 0.4
        color.g = 0.5
        color.b = 0.6

        GetSetCompareTest(component, path_color, color)

        # Tests for BuildComponentPropertyTreeEditor
        pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', component)
        check_result(pteObj.IsSuccess(), "BuildComponentPropertyTreeEditor")
     
        pte = pteObj.GetValue()

        PteTest(pte, path_visible, True)
        PteTest(pte, path_quad_width, 48.0)

        color = math.Color()
        color.r = 0.9
        color.g = 0.1
        color.b = 0.3

        PteTest(pte, path_color, color)

if __name__ == "__main__":
    tester = Editor_ComponentPropertyCommands_Works()
    tester.test_case(tester.test)

