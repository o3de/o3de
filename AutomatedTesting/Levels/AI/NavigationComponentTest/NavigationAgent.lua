----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local NavigationAgent = {
    Properties = {
        Goal=EntityId(),
        CustomMovement=false
    }
}

function NavigationAgent:OnActivate()
    self.listener = NavigationComponentNotificationBus.Connect(self, self.entityId)
    self.name = GameEntityContextRequestBus.Broadcast.GetEntityName(self.entityId)
    Debug.Log("OnActivate " .. self.name)

    NavigationComponentRequestBus.Event.FindPathToEntity(self.entityId, self.Properties.Goal)
end

function NavigationAgent:OnTraversalCancelled(requestId)
    Debug.Log("OnTraversalCancelled ".. self.name)
end

function NavigationAgent:OnTraversalPathUpdate(requestId, nextPathPosition, inflectionPosition)
    if self.Properties.CustomMovement then
        Debug.Log("OnTraversalPathUpdate ".. self.name)
        TransformBus.Event.SetWorldTranslation(self.entityId, nextPathPosition)
    end
end
function NavigationAgent:OnTraversalComplete(requestId)
    Debug.Log("OnTraversalComplete ".. self.name)
end

function NavigationAgent:OnDeactivate()
    self.listener:Disconnect()
    Debug.Log("OnDeactivate " .. self.name)
end

return NavigationAgent
