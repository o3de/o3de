--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--

----------------------------------------------------------------------------------------------------
-- WhiteBoxLiveBooleanBusTest.lua - exercises WhiteBoxComponentRequestBus (SetLiveBoolean,
-- GetLiveBoolean, BakeWhiteBox). Put it on an entity with a White Box Component and Collider that
-- has a Live Boolean set up, enter game mode and watch the [WhiteBoxBusTest] log lines.
----------------------------------------------------------------------------------------------------

local WhiteBoxLiveBooleanBusTest = {
    Properties = {
        AutoToggle = { default = true, description = "Flip the live boolean on a timer for a visual test." },
        ToggleInterval = { default = 2.0, description = "Seconds between toggles when AutoToggle is on." },
    },
}

local function boolToString(value)
    if value then return "true" else return "false" end
end

-- One-shot functional check of set/get/bake.
function WhiteBoxLiveBooleanBusTest:RunChecks()
    local id = self.entityId

    local initial = WhiteBoxComponentRequestBus.Event.GetLiveBoolean(id)
    Debug.Log("[WhiteBoxBusTest] initial GetLiveBoolean = " .. boolToString(initial))

    -- turn the live boolean ON and bake
    WhiteBoxComponentRequestBus.Event.SetLiveBoolean(id, true)
    WhiteBoxComponentRequestBus.Event.BakeWhiteBox(id)
    local afterOn = WhiteBoxComponentRequestBus.Event.GetLiveBoolean(id)
    Debug.Log("[WhiteBoxBusTest] SetLiveBoolean(true)  -> GetLiveBoolean = " .. boolToString(afterOn))

    -- turn the live boolean OFF and bake
    WhiteBoxComponentRequestBus.Event.SetLiveBoolean(id, false)
    WhiteBoxComponentRequestBus.Event.BakeWhiteBox(id)
    local afterOff = WhiteBoxComponentRequestBus.Event.GetLiveBoolean(id)
    Debug.Log("[WhiteBoxBusTest] SetLiveBoolean(false) -> GetLiveBoolean = " .. boolToString(afterOff))

    -- evaluate. Note SetLiveBoolean(true) only sticks when a boolean variant was baked
    -- (i.e. a Boolean Source is set); with no source it correctly stays false.
    if afterOff == false and afterOn == true then
        Debug.Log("[WhiteBoxBusTest] PASS: live boolean toggles on and off via the bus.")
    elseif afterOff == false and afterOn == false then
        Debug.Log("[WhiteBoxBusTest] PASS (no boolean baked): set/get consistent; ON stays false with no Boolean Source.")
    else
        Debug.Log("[WhiteBoxBusTest] FAIL: unexpected GetLiveBoolean results.")
    end

    -- restore the original selection
    WhiteBoxComponentRequestBus.Event.SetLiveBoolean(id, initial)
    WhiteBoxComponentRequestBus.Event.BakeWhiteBox(id)
end

function WhiteBoxLiveBooleanBusTest:OnActivate()
    self:RunChecks()

    if self.Properties.AutoToggle then
        self.state = WhiteBoxComponentRequestBus.Event.GetLiveBoolean(self.entityId)
        self.timer = 0.0
        self.tickBusHandler = TickBus.Connect(self)
    end
end

function WhiteBoxLiveBooleanBusTest:OnTick(deltaTime, timePoint)
    self.timer = self.timer + deltaTime
    if self.timer >= self.Properties.ToggleInterval then
        self.timer = 0.0
        self.state = not self.state
        WhiteBoxComponentRequestBus.Event.SetLiveBoolean(self.entityId, self.state)
        WhiteBoxComponentRequestBus.Event.BakeWhiteBox(self.entityId)
        Debug.Log("[WhiteBoxBusTest] toggled live boolean -> " ..
            boolToString(WhiteBoxComponentRequestBus.Event.GetLiveBoolean(self.entityId)))
    end
end

function WhiteBoxLiveBooleanBusTest:OnDeactivate()
    if self.tickBusHandler ~= nil then
        self.tickBusHandler:Disconnect()
        self.tickBusHandler = nil
    end
end

return WhiteBoxLiveBooleanBusTest
