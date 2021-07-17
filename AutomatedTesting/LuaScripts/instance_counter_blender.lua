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
local InstanceCounterBlender = 
{
    Properties =
    {
    }
}

function InstanceCounterBlender:OnActivate()
    self.tickBusHandler = TickBus.CreateHandler(self)
    self.tickBusHandler:Connect()
    self.elapsed = 0
end

function InstanceCounterBlender:OnTick(deltaTime, timePoint)
    self.elapsed = self.elapsed + deltaTime
    if self.elapsed >= 3 then
        self:ValidateInstances()
    end
end

function InstanceCounterBlender:ValidateInstances(instances)
    self.tickBusHandler:Disconnect()
    Debug.Log("Verifying instances have properly spawned...")
    num_instances_found = 0
    while num_instances_found ~= 400 do
        num_instances_found = AreaBlenderRequestBus.Broadcast.GetAreaProductCount(self.entityId)
        Debug.Log("Number of instances found = " .. num_instances_found)
    end
    box = ShapeComponentRequestsBus.Event.GetEncompassingAabb(self.entityId)
    instances = AreaSystemRequestBus.Broadcast.GetInstancesInAabb(box)
    if num_instances_found == #instances then
        purple_count = 0
        pink_count = 0
        Debug.Log("Number of instances found in Aabb = " .. #instances)
        for idx=1, #instances do
            local purple_path = "slices/purpleflower.dynamicslice"
            local pink_path = "slices/pinkflower.dynamicslice"
            local instance_asset_path = instances[idx].descriptor.spawner:GetSliceAssetPath()
            if instance_asset_path == purple_path then
                purple_count = purple_count + 1
            elseif instance_asset_path == pink_path then
                pink_count = pink_count + 1
            end
        end
    else
        Debug.Log("Number of instances found does not match expected instances! Expected: "  .. num_instances_found .. ", Found:" .. #instances)
    end
    local success = purple_count == pink_count
    Debug.Log("Test Success = " .. tostring(success))
    if success then
        ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("quit")
    end
end

function InstanceCounterBlender:OnDeactivate()

end


return InstanceCounterBlender