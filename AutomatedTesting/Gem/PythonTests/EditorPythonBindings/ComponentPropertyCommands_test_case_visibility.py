"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# ComponentPropertyCommands test case visibility

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity

# Open a level
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'ocean_component')

def print_result(result, message):
    if result:
        print(message + ": SUCCESS")
    else:
        print(message + ": FAILURE")

def add_componet(typename, entityId):
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [typename], entity.EntityType().Game)
    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
    print_result(componentOutcome.IsSuccess(), "{} component added to entity".format(typename))
    return componentOutcome.GetValue()

# Find ocean entity
searchFilter = entity.SearchFilter()
searchFilter.names = ["the_ocean"]
oceanEntityId = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)[0]
print_result(oceanEntityId, "oceanEntityId was found")

# Find Infinite Ocean component
typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ['Infinite Ocean'], entity.EntityType().Game)
getComponentOutcome =  editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', oceanEntityId, typeIdsList[0])
print_result(getComponentOutcome.IsSuccess(), "Found Infinite Ocean component ID")
infiniteOceanId = getComponentOutcome.GetValue()

# Get the PTE from the Mesh Component
pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', infiniteOceanId)
print_result(pteObj.IsSuccess(), "Created a PropertyTreeEditor for the infiniteOceanId")
pte = pteObj.GetValue()

# test for visibility (default all nodes are exposed)
print_result(pte.get_value('m_data|General|General|Enable Ocean Bottom').IsSuccess(), "Found proprety hidden node in path")

# enable visibility enforcement
pte.set_visible_enforcement(True)
print_result(pte.get_value('m_data|General|General|Enable Ocean Bottom').IsSuccess() is not True, "Proprety node is now a hidden path")

# test for visibility (missing some properties now)
print_result(pte.get_value('General|Enable Ocean Bottom').IsSuccess(), "Property path enforcement of visibility")

# All Done!
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
