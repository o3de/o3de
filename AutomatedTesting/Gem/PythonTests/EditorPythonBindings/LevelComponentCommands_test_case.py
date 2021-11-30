"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Component CRUD Python API while the Editor is running

import sys
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
from azlmbr.entity import EntityType

def validate_component_property_apis(levelEntityComponentIdPair, componentName, componentUuid):
    propertyTreeOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', levelEntityComponentIdPair)
    if not propertyTreeOutcome.IsSuccess():
        print("ERROR: BuildComponentPropertyTreeEditor failed for component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    pte = propertyTreeOutcome.GetValue()
    propList = pte.build_paths_list()
    propList2 = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', levelEntityComponentIdPair)
    if len(propList) != len(propList2):
        print("ERROR: len(propList)={} != len(propList2)={} for component with name={}, uuid={}".format(len(propList), len(propList2), componentName, componentUuid))
        return False
    for propName in propList2:
        #Some components like the Comment component have empty string property.
        if propName == "":
            continue
        propOutcome =  editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', levelEntityComponentIdPair, propName)
        if not propOutcome.IsSuccess():
            print("ERROR: Failed to get component property={} for component with name={}, uuid={}".format(propName, componentName, componentUuid))
            return False
        if not propName in propList2:
            print("ERROR: Failed to find component property={} in propList2 for component with name={}, uuid={}".format(propName, componentName, componentUuid))
            return False
    return True

def validate_level_component_api(componentName, componentUuid):
    if componentName == "PhysX Terrain":
        #Skip physX. It has issues now that the Legacy Terrain is a component.
        return True
    if editor.EditorLevelComponentAPIBus(bus.Broadcast, 'HasComponentOfType', componentUuid):
        print("ERROR: Component with name={}, uuid={} was already present".format(componentName, componentUuid))
        return False
    if editor.EditorLevelComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', componentUuid) > 0:
        print("ERROR: Component with name={}, uuid={} was already present".format(componentName, componentUuid))
        return False       
    addComponentsOutcome = editor.EditorLevelComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', [componentUuid])
    if not addComponentsOutcome.IsSuccess():
        print("ERROR: Failed to add Component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    componentIds = addComponentsOutcome.GetValue()
    if len(componentIds) != 1:
        print("ERROR: Expecting 1 added Component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    print("SUCCESS: Added 1 Component with name={}, uuid={}".format(componentName, componentUuid))
    general.idle_wait(3.0)
    if not editor.EditorLevelComponentAPIBus(bus.Broadcast, 'HasComponentOfType', componentUuid):
        print("ERROR: Component with name={}, uuid={} was not present".format(componentName, componentUuid))
        return False
    if editor.EditorLevelComponentAPIBus(bus.Broadcast, 'CountComponentsOfType', componentUuid) != 1:
        print("ERROR: Was expecting 1 Component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    getComponentOutcome = editor.EditorLevelComponentAPIBus(bus.Broadcast, 'GetComponentOfType', componentUuid)
    if not getComponentOutcome.IsSuccess():
        print("ERROR: Failed to get added component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    componentId = getComponentOutcome.GetValue()
    if not componentId.Equal(componentIds[0]):
        print("ERROR. GetComponentOfType no matching for component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    getComponentsOutcome = editor.EditorLevelComponentAPIBus(bus.Broadcast, 'GetComponentsOfType', componentUuid)
    newComponentIds = getComponentsOutcome.GetValue()
    if len(newComponentIds) != 1:
        print("ERROR: Expecting to get 1 Component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    if not componentIds[0].Equal(newComponentIds[0]):
        print("ERROR. GetComponentsOfType no matching for component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    if not editor.EditorComponentAPIBus(bus.Broadcast, 'IsValid', componentId):
        print("ERROR. expecting valid component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    if not editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', componentId):
        if not editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', [componentId]):
            print("ERROR. Failed to pre-enable component with name={}, uuid={}".format(componentName, componentUuid))
            return False
        print("SUCCESS: pre-enabled component with name={}, uuid={} ".format(componentName, componentUuid))
        general.idle_wait(3.0)
        
    if not editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', componentId):
        print("ERROR. Expecting enabled component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    if not editor.EditorComponentAPIBus(bus.Broadcast, 'DisableComponents', [componentId]):
        print("ERROR. Failed to disable component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    print("SUCCESS: Disabled component with name={}, uuid={} ".format(componentName, componentUuid))
    general.idle_wait(1.0)
    if editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', componentId):
        print("ERROR. Expecting disabled component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    if not editor.EditorComponentAPIBus(bus.Broadcast, 'EnableComponents', [componentId]):
        print("ERROR. Failed to re-enable component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    print("SUCCESS: Re-enabled component with name={}, uuid={} ".format(componentName, componentUuid))
    general.idle_wait(3.0)

    if not validate_component_property_apis(componentId, componentName, componentUuid):
        print("ERROR. Failed to validate_component_property_apis for component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    
    if not editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [componentId]):
        print("ERROR. Failed to remove component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    print("SUCCESS: Removed component with name={}, uuid={} ".format(componentName, componentUuid))
    general.idle_wait(1.0)
    if editor.EditorLevelComponentAPIBus(bus.Broadcast, 'HasComponentOfType', componentUuid):
        print("ERROR: Was not expecting to have Component with name={}, uuid={}".format(componentName, componentUuid))
        return False
    print("SUCCESS: validated API with component with name={}, uuid={} ".format(componentName, componentUuid))
    return True

exitWhenDone = False
if len(sys.argv) > 1:
    if sys.argv[1] == "exit_when_done":
        exitWhenDone = True

# Open a level (any level should work)
general.create_level_no_prompt('LevelComponentTest', 128, 1, 512, True)
# Make sure the default slices get a chance to initialize.
general.idle_wait(1.0)

# Generate List of Component Types
componentList = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameListByEntityType', EntityType().Level)

if(len(componentList) > 0):
    print("Component List returned correctly")

# Get Component Types for all level components
typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', componentList, EntityType().Level)

if len(typeIdsList) != len(componentList):
    print("ERROR: length of typeIdsList={} and length of componentList={}".format(len(typeIdsList), len(componentList)))
    general.exit_no_prompt()

print("Type Ids List returned correctly")

# Get Component names from Component Types
typeNamesList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeNames', typeIdsList)

if len(typeIdsList) != len(typeNamesList):
    print("ERROR: length of typeIdsList={} and length of typeNamesList={}".format(len(typeIdsList), len(typeNamesList)))
    general.exit_no_prompt()

for i in range(len(componentList)):
    if componentList[i] != typeNamesList[i]:
        print("ERROR: componentList[{}]({}) != typeNamesList=[{}]({})".format(i, componentList[i], i, typeNamesList[i]))
        general.exit_no_prompt()

print("Type Names List returned correctly")

#Let's add each level component, one at a time, wait, remove component
for i in range(len(componentList)):
    if validate_level_component_api(componentList[i], typeIdsList[i]):
        continue
    print("ERROR: Failed to validate_level_component_api for component with name={}, uuid={}".format(componentList[i], typeIdsList[i]))
    general.exit_no_prompt()

print("Level Component API validated")

if exitWhenDone:
    general.exit_no_prompt()