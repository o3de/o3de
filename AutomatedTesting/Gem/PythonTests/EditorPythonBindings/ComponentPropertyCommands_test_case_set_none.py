"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests setting property values that are not 32 or 64 bit such as a u8

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math

# Open a level
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'ocean_component')

# toggle the entity system to use visibility enforcement rules
editor.EditorComponentAPIBus(bus.Broadcast, 'SetVisibleEnforcement', True)

def print_result(message, result):
    if result:
        print(message + ": SUCCESS")
    else:
        print(message + ": FAILURE")

def get_entity_by_name(name):
    searchFilter = entity.SearchFilter()
    searchFilter.names = [name]
    searchResult = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
    if(len(searchResult) > 0):
        return searchResult
    print_result("get_entity_by_name - {}".format(name), False)
    return None

def get_component_type_by_name(name):
    gameType = entity.EntityType().Game
    listOfTypeIds = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [name], gameType)
    if(len(listOfTypeIds) > 0):
        return listOfTypeIds[0]
    print_result("get_component_type_by_name - {}".format(name), False)
    return None    

def get_component_of_type(entity, componentTypeId):
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity, componentTypeId)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    print_result("get_component_of_type - {} for type {}".format(entity, componentTypeId), False)
    return None

def get_component_property(component, path):
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
    if(outcome.IsSuccess()):
        return outcome.GetValue()
    print_result("get_component_property - {}".format(path), False)
    return None

def set_component_property(component, path, value):
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
    if(outcome.IsSuccess()):
        return True
    print_result("set_component_property - {} with value {}".format(path, value), False)
    return False

# fetch the ocean entity and its 'ocean' component
entityId = get_entity_by_name('the_ocean')[0]
riverComponentTypeId = get_component_type_by_name('Infinite Ocean')
riverComponent = get_component_of_type(entityId, riverComponentTypeId)

# this series tests that a material can be fetched, mutated with a None, and reverts to an 'empty asset'
materialPropertPath = 'General|Water Material'
material = get_component_property(riverComponent, materialPropertPath)
print_result("material current is valid - {}".format(material.is_valid()), material.is_valid())

materialUpdateResult = set_component_property(riverComponent, materialPropertPath, None)
print_result("material set to None", materialUpdateResult)

material = get_component_property(riverComponent, materialPropertPath)
print_result("material has been set to None", material.is_valid() is False)

# All Done!
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
