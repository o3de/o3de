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

local ChildDropTargets_LayoutDropTarget = 
{
    Properties = 
    {
        EndDropTarget = {default = EntityId()},
    },
}

function ChildDropTargets_LayoutDropTarget:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function ChildDropTargets_LayoutDropTarget:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function ChildDropTargets_LayoutDropTarget:OnDropHoverStart(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
end

function ChildDropTargets_LayoutDropTarget:OnDropHoverEnd(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
end

function ChildDropTargets_LayoutDropTarget:OnDrop(draggable)
    -- insert before the invisible end drop target
    UiElementBus.Event.Reparent(draggable, self.entityId, self.Properties.EndDropTarget)
end

return ChildDropTargets_LayoutDropTarget