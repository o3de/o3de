"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests setting property values that are not 32 or 64 bit such as a u8

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math

# Open a level
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'auto_test')

def print_result(message, result):
    if result:
        print(message + ": SUCCESS")
    else:
        print(message + ": FAILURE")

def GetSetCompareTest(component, path, value):
    oldObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
    if(oldObj.IsSuccess()):
        oldValue = oldObj.GetValue()

    oldValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)
    print_result("CompareComponentProperty - {}".format(path), oldValueCompared)
    editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
    newObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
    if(newObj.IsSuccess()):
        newValue = newObj.GetValue()

    newValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, newValue)
    isOldNewValueSame = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)
    result = not(newValue == oldValue and newValueCompared and not isOldNewValueSame)
    print_result("GetSetCompareTest - {}".format(path), result)

# Create new Entity
entity_position = math.Vector3(125.0,136.0,32.0)
entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntityAtPosition', entity_position, entity.EntityId())
print_result("New entity with no parent created", entityId is not None)

# create a vegetation layer with a box shape and distance filter
typenameList = ["Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape", "Vegetation Distance Between Filter"]
typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', typenameList, entity.EntityType().Game)
addComponentsOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
print_result("Components added to entity", addComponentsOutcome.IsSuccess())

# fetch the Vegetation Distance Between Filter
vegDistTypeIdList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Vegetation Distance Between Filter"], entity.EntityType().Game)
componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentsOfType', entityId, vegDistTypeIdList[0])
print_result('Found Vegetation Distance Between Filter', componentOutcome.IsSuccess())

paths = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', componentOutcome.GetValue()[0])

# update the bound box type
pathToBoundMode = "Configuration|Bound Mode"
GetSetCompareTest(componentOutcome.GetValue()[0], pathToBoundMode, 1)

# All Done!
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
