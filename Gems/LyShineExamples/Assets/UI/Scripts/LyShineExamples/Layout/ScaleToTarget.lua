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

local ScaletoTarget =
{
    Properties =
    {
        Target = {default = EntityId()},
    },
}

function ScaletoTarget:OnActivate()
    self.tickHandler = TickBus.Connect(self)
end

function ScaletoTarget:OnTick()
    local targetOffsets = UiTransform2dBus.Event.GetOffsets(self.Properties.Target)
    UiTransform2dBus.Event.SetOffsets(self.entityId, targetOffsets)
end

function ScaletoTarget:OnDeactivate()
    self.tickHandler:Disconnect()
end

return ScaletoTarget