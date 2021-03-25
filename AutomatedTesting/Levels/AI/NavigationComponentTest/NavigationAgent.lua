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
