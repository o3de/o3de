-- Just  basic template that you can quickly copy/paste
-- and start writing your component class
local dynamic_obj_trigger_counter = {
    Properties = {
        waitTime = { default = 3.0, suffix="s", description="Time to wait before first spawn."},
    }
}

function dynamic_obj_trigger_counter:OnActivate()
    -- Debug.Log("dynamic_obj_trigger_counter:OnActivate=" .. tostring(self.Properties.spawnable))

    require("Levels.goal_9ks_1kd.Scripts.ScriptEvents.DynamicObjTriggerNotificationBus")

    self._elapsedTime = 0.0
	-- Connect to tick bus to receive time updates, during these ticks
	-- we will dispatch all enqueued events.
	self.tickBusHandler = TickBus.Connect(self)
end

function dynamic_obj_trigger_counter:OnDeactivate()
    -- Crashes the game/editor.
    -- self._spawnableScriptMediator:Despawn(self._ticket)

    self:_UnregisterTriggerEvents()

	if self.tickBusHandler then
		self.tickBusHandler:Disconnect()
	end

end


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Public methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Public methods END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- TickBus  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function dynamic_obj_trigger_counter:OnTick(deltaTime, timePoint)
    self._elapsedTime = self._elapsedTime + deltaTime
    if self._elapsedTime < self.Properties.waitTime then
        return
    end
    Debug.Log("dynamic_obj_trigger_counter:OnTick _RegisterTriggerEvents")
    self:_RegisterTriggerEvents()

    self.tickBusHandler:Disconnect()
    self.tickBusHandler = nil
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- TickBus END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
-- Private methods  START
-->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
function dynamic_obj_trigger_counter:_RegisterTriggerEvents()
    self._onTriggerEnterHandler = SimulatedBody.GetOnTriggerEnterEvent(self.entityId):Connect(
        function(tuple, triggerEvent)
            --Debug.Log("dynamic_obj_trigger_counter: On Enter")
            -- TagComponentRequestBus.Event.HasTag
            local trigger = triggerEvent:GetTriggerEntityId()
            local other = triggerEvent:GetOtherEntityId()
            assert(trigger == self.entityId, "Was expecting trigger and self.entityId to be the same.")
            --Debug.Log("trigger=" .. tostring(trigger) .. ", other=" .. tostring(other) .. ", myId=" .. tostring(self.entityId))
            DynamicObjTriggerNotificationBus.Broadcast.OnEntityEntered(other)

        end
    )

    self._onTriggerExitHandler = SimulatedBody.GetOnTriggerExitEvent(self.entityId):Connect(
        function(tuple, triggerEvent)
            --Debug.Log("dynamic_obj_trigger_counter: On Exit")
            local trigger = triggerEvent:GetTriggerEntityId()
            local other = triggerEvent:GetOtherEntityId()
            assert(trigger == self.entityId, "Was expecting trigger and self.entityId to be the same.")
            --Debug.Log("trigger=" .. tostring(trigger) .. ", other=" .. tostring(other) .. ", myId=" .. tostring(self.entityId))
            DynamicObjTriggerNotificationBus.Broadcast.OnEntityExited(other)
        end
    )
end


function dynamic_obj_trigger_counter:_UnregisterTriggerEvents()
    if self._onTriggerEnterHandler then
        self._onTriggerEnterHandler:Disconnect()
        self._onTriggerEnterHandler = nil
    end
    if self._onTriggerExitHandler then
        self._onTriggerExitHandler:Disconnect()
        self._onTriggerExitHandler = nil
    end
end

--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Private methods END
--<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


return dynamic_obj_trigger_counter
