----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------

local ChildDropTargets_EndDropTarget = 
{
    Properties = 
    {
    },
}

function ChildDropTargets_EndDropTarget:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function ChildDropTargets_EndDropTarget:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function ChildDropTargets_EndDropTarget:OnDropHoverStart(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
end

function ChildDropTargets_EndDropTarget:OnDropHoverEnd(draggable)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
end

function ChildDropTargets_EndDropTarget:OnDrop(draggable)
    local parentLayout = UiElementBus.Event.GetParent(self.entityId)
    -- add before this end marker
    UiElementBus.Event.Reparent(draggable, parentLayout, self.entityId)
end

return ChildDropTargets_EndDropTarget