"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests a portion of the Component Property Get/Set Python API while the Editor is running

import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.entity as entity
import azlmbr.editor as editor
import azlmbr.math as math


# Create a test level
general.create_level_no_prompt("LightningArcTestLevel", 1024, 1, 1024, True)

def ChangeProperty(component, path, value):
    getPropertyOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
    if not(getPropertyOutcome.IsSuccess()):
        print("GetComponentProperty " + path + " failed.")
    else:
        oldValue = getPropertyOutcome.GetValue()

    setPropertyOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
    if not(setPropertyOutcome.IsSuccess()):
        print("SetComponentProperty " + path + " to " + str(value) + " failed.")

    getPropertyOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
    if not(getPropertyOutcome.IsSuccess()):
        print("GetComponentProperty " + path + " failed.")
    else:
        newValue = getPropertyOutcome.GetValue()

    if not(newValue == oldValue):
        print("ChangeProperty " + path + " to " + str(value) + " succeeded.")
    else:
        print("ChangeProperty " + path + " to " + str(value) + " failed.")

# Create new Entity
entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

if (entityId.IsValid()):
    print("Created new entity.")

# Get Component Type for Lightning Arc
typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Lightning Arc"], entity.EntityType().Game)

componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)

if (componentOutcome.IsSuccess()):
    print("Lightning Arc component added to entity.")

components = componentOutcome.GetValue()
component = components[0]

# Tests for GetComponentProperty/SetComponentProperty
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 0)
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 1)
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 25)
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 50)
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 70)
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 75)
ChangeProperty(component, "m_config|Arc Parameters|Segment Count", 100)

ChangeProperty(component, "m_config|Arc Parameters|Point Count", 0)
ChangeProperty(component, "m_config|Arc Parameters|Point Count", 1)
ChangeProperty(component, "m_config|Arc Parameters|Point Count", 25)
ChangeProperty(component, "m_config|Arc Parameters|Point Count", 50)
ChangeProperty(component, "m_config|Arc Parameters|Point Count", 70)
ChangeProperty(component, "m_config|Arc Parameters|Point Count", 75)
ChangeProperty(component, "m_config|Arc Parameters|Point Count", 100)

general.exit_no_prompt()
