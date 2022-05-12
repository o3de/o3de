"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Component Property Get/Set Python API while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math

# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'Base')

def GetSetCompareTest(component, path, value):
    oldObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)

    if(oldObj.IsSuccess()):
        oldValue = oldObj.GetValue()

    oldValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)

    editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)

    newObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)

    if(newObj.IsSuccess()):
        newValue = newObj.GetValue()

    newValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, newValue)

    isOldNewValueSame = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)

    if not(newValue == oldValue and oldValueCompared and newValueCompared and not isOldNewValueSame):
        print("GetSetCompareTest " + path + ": SUCCESS")
    else:
        print("GetSetCompareTest " + path + ": FAILURE")

def PteTest(pte, path, value):
    oldObj = pte.get_value(path)

    if(oldObj.IsSuccess()):
        oldValue = oldObj.GetValue()
    oldValueCompared = pte.compare_value(path, oldValue)

    pte.set_value(path, value)

    newObj = pte.get_value(path)

    if(newObj.IsSuccess()):
        newValue = newObj.GetValue()

    newValueCompared = pte.compare_value(path, newValue)

    isOldNewValueSame = pte.compare_value(path, oldValue)

    if not(newValue == oldValue and oldValueCompared and newValueCompared and not isOldNewValueSame):
        print("PteTest " + path + ": SUCCESS")
    else:
        print("PteTest " + path + ": FAILURE")

# Create new Entity
entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

if (entityId):
    print("New entity with no parent created")

# Get Component Type for Environment Probe
typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Environment Probe"], entity.EntityType().Game)

componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)

if (componentOutcome.IsSuccess()):
    print("Environment Probe component added to entity")

components = componentOutcome.GetValue()
component = components[0]

hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entityId, typeIdsList[0])

if(hasComponent):
    print("Entity has an Environment Probe component")

# Test BuildComponentPropertyList
paths = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', component)

if(len(paths) > 0):
    print("get_paths_list works")

# Tests for GetComponentProperty/SetComponentProperty

GetSetCompareTest(component, "Settings|General Settings|Visible", False)
GetSetCompareTest(component, "Settings|Animation|Style", 2.0)
GetSetCompareTest(component, "Settings|Environment Probe Settings|Box height", 42)

color = math.Color()
color.r = 0.4
color.g = 0.5
color.b = 0.6

GetSetCompareTest(component, "Settings|General Settings|Color", color)

vec3 = math.Vector3()
vec3.x = 1.0
vec3.y = 2.0
vec3.z = 3.0

GetSetCompareTest(component, "Settings|Environment Probe Settings|Area dimensions", vec3)

# Tests for BuildComponentPropertyTreeEditor
pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', component)

if(pteObj.IsSuccess()):
    print("")
    pte = pteObj.GetValue()

PteTest(pte, "Settings|General Settings|Visible", True)
PteTest(pte, "Settings|Animation|Style", 4)
PteTest(pte, "Settings|Environment Probe Settings|Box height", 48.0)

color = math.Color()
color.r = 0.9
color.g = 0.1
color.b = 0.3

PteTest(pte, "Settings|General Settings|Color", color)

vec3 = math.Vector3()
vec3.x = 7.0
vec3.y = 4.0
vec3.z = 1.0

PteTest(pte, "Settings|Environment Probe Settings|Area dimensions", vec3)

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
