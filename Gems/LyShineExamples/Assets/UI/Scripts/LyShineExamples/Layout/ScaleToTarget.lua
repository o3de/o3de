----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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
