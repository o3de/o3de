"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests component properties that are containers

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.surface_data
import azlmbr.globals

# Open a level
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'auto_test')

def print_result(message, result):
    if result:
        print(message + ": SUCCESS")
    else:
        print(message + ": FAILURE")

def is_container(pte, path):
    return pte.is_container(path)

def get_container_count(pte, path):
    outcome = pte.get_container_count(path)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    return False

def reset_container(pte, path):
    outcome = pte.reset_container(path)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    return False

def add_container_item(pte, path, key, item):
    outcome = pte.add_container_item(path, key, item)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    return False

def remove_container_item(pte, path, key):
    outcome = pte.remove_container_item(path, key)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    return False

def update_container_item(pte, path, key, value):
    outcome = pte.update_container_item(path, key, value)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    return False

def get_container_item(pte, path, key):
    outcome = pte.get_container_item(path, key)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    return False

# Create new Entity
entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
print_result("New entity with no parent created", entityId)

tagOne = azlmbr.surface_data.SurfaceTag()
tagOne.SetTag('one')

tagTwo = azlmbr.surface_data.SurfaceTag()
tagTwo.SetTag('two')

tagThree = azlmbr.surface_data.SurfaceTag()
tagThree.SetTag('three')

tagFour = azlmbr.surface_data.SurfaceTag()
tagFour.SetTag('four')

# create a component with a TagSurface container
typeIdsList = [azlmbr.globals.property.GradientSurfaceDataComponentTypeId]

componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
if (componentOutcome.IsSuccess()):
    print("GradientSurfaceDataComponent added to entity :SUCCESS")
else:
    raise Exception('FAILURE FATAL: AddComponentsOfType')

components = componentOutcome.GetValue()
tagList = components[0]

pteOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', tagList)
if(pteOutcome.IsSuccess()):
    pte = pteOutcome.GetValue()
    print("Created a PropertyTreeEditor :SUCCESS")
else:
    raise Exception('FAILURE FATAL: BuildComponentPropertyTreeEditor')

# Test BuildComponentPropertyList
paths = pte.build_paths_list()
for p in paths:
    print('>> {}'.format(p))

tagListPropertyPath = 'm_template|Extended Tags'

print_result('Has a container', is_container(pte, tagListPropertyPath))
print_result('Has zero items', get_container_count(pte, tagListPropertyPath) == 0)
print_result('Add an item 0', add_container_item(pte, tagListPropertyPath, 0, tagOne))
print_result('Has one item 0', get_container_count(pte, tagListPropertyPath) == 1)
print_result('Add an item 1', add_container_item(pte, tagListPropertyPath, 1, tagOne))
print_result('Add an item 2', add_container_item(pte, tagListPropertyPath, 2, tagTwo))
print_result('Add an item 3', add_container_item(pte, tagListPropertyPath, 3, tagThree))
print_result('Has four items', get_container_count(pte, tagListPropertyPath) == 4)
print_result('Updated an item', update_container_item(pte, tagListPropertyPath, 2, tagFour))

itemTag = get_container_item(pte, tagListPropertyPath, 2)
print_result ('itemTag equals tagFour', itemTag.Equal(tagFour))

print_result('Removed one item 0', remove_container_item(pte, tagListPropertyPath, 0))
print_result('Removed one item 1', remove_container_item(pte, tagListPropertyPath, 0))
print_result('Has two items', get_container_count(pte, tagListPropertyPath) == 2)
print_result('Reset items', reset_container(pte, tagListPropertyPath))
print_result('Has cleared the items', get_container_count(pte, tagListPropertyPath) == 0)

# All Done!
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
