"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Entity CRUD Python API while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
from azlmbr.entity import EntityId

# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'Base')

parentEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
childEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

# Test SetParent/GetParent

queryParent = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', childEntityId)
if(queryParent.ToString() == EntityId().ToString()):
    print("childEntity is parented to the Root")

editor.EditorEntityAPIBus(bus.Event, 'SetParent', childEntityId, parentEntityId)

queryParent = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', childEntityId)
if(queryParent.ToString() == parentEntityId.ToString()):
    print("childEntity is now parented to the parentEntity")


# Test SetLockState and IsLocked

queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', parentEntityId)
if(queryLock == False):
    print("parentEntityId isn't locked")

editor.EditorEntityAPIBus(bus.Event, 'SetLockState', parentEntityId, True)

queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', parentEntityId)
if(queryLock == True):
    print("parentEntityId is now locked")

queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', childEntityId)
if(queryLock == True):
    print("childEntityId is now locked too")


# Test SetVisibilityState, IsVisible and IsHidden

queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', parentEntityId)
if(queryVisibility == True):
    print("parentEntityId is visible")

queryHidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', parentEntityId)
if(queryHidden == False):
    print("parentEntityId isn't hidden")

editor.EditorEntityAPIBus(bus.Event, 'SetVisibilityState', parentEntityId, False)

queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', parentEntityId)
if(queryVisibility == False):
    print("parentEntityId is now hidden")

queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', childEntityId)
if(queryVisibility == False):
    print("childEntityId is now hidden too")


# Test EditorOnly

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
print("EditorOnly before queryStatus: " + str(queryStatus))

if not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly):
    print("parentEntityId isn't set to Editor Only")

editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', parentEntityId, azlmbr.globals.property.EditorEntityStartStatus_EditorOnly)

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
print("EditorOnly after queryStatus: " + str(queryStatus))

if (queryStatus == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly):
    print("parentEntityId is now set to Editor Only")

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', childEntityId)
if not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly):
    print("childEntityId does not inherit Editor Only")


# Test StartInactive

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
print("queryStatus: " + str(queryStatus))

if not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartInactive):
    print("parentEntityId isn't set to Start Inactive")

editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', parentEntityId, azlmbr.globals.property.EditorEntityStartStatus_StartInactive)

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
if (queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartInactive):
    print("parentEntityId is now set to Start Inactive")

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', childEntityId)
if not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartInactive):
    print("childEntityId does not inherit Start Inactive")


# Test StartActive

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
print("queryStatus: " + str(queryStatus))

if not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartActive):
    print("parentEntityId isn't set to Start Active")

editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', parentEntityId, azlmbr.globals.property.EditorEntityStartStatus_StartActive)

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
if (queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartActive):
    print("parentEntityId is now set to Start Active")

queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', childEntityId)
if (queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartActive):
    print("childEntityId should still be set to Start Active by default")


editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
