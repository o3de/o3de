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

local FindMaterialAssignmentTest = 
{
    Properties = 
    {
        Textures = 
        {
            "materials/presets/macbeth/05_blue_flower_srgb.tif.streamingimage",
            "materials/presets/macbeth/06_bluish_green_srgb.tif.streamingimage",
            "materials/presets/macbeth/09_moderate_red_srgb.tif.streamingimage",
            "materials/presets/macbeth/11_yellow_green_srgb.tif.streamingimage",
            "materials/presets/macbeth/12_orange_yellow_srgb.tif.streamingimage",
            "materials/presets/macbeth/17_magenta_srgb.tif.streamingimage"
        }, 
        MaterialSlotFilter = ""
    }, 
}

function randomColor()
    return Color(math.random(), math.random(), math.random(), 1.0)
end

function randomDir()
    dir = {}
    for i = 1, 3 do
        lerpDir = math.random()
        if lerpDir < 0.5 then
            table.insert(dir, -1.0)
        else
            table.insert(dir, 1.0)
        end
    end
    return dir
end

function FindMaterialAssignmentTest:OnActivate()
    self.timer = 0.0
    self.totalTime = 0.0
    self.totalTimeMax = 200.0
    self.timeUpdate = 2.0
    self.colors = {}
    self.lerpDirs = {}

    self.tickBusHandler = TickBus.Connect(self);
end

function FindMaterialAssignmentTest:UpdateFactor(assignmentId)
    local propertyName = "baseColor.factor"
    local propertyValue = math.random()
    MaterialComponentRequestBus.Event.SetPropertyOverride(self.entityId, assignmentId, propertyName, propertyValue);
end

function FindMaterialAssignmentTest:UpdateColor(assignmentId, color)
    local propertyName = "baseColor.color"
    local propertyValue = color
    MaterialComponentRequestBus.Event.SetPropertyOverride(self.entityId, assignmentId, propertyName, propertyValue);
end

function FindMaterialAssignmentTest:UpdateTexture(assignmentId)
    if (#self.Properties.Textures > 0) then
        local propertyName = "baseColor.textureMap"
        local textureName = self.Properties.Textures[ math.random( #self.Properties.Textures ) ]
        Debug.Log(textureName)
        local textureAssetId = AssetCatalogRequestBus.Broadcast.GetAssetIdByPath(textureName, Uuid(), false)
        MaterialComponentRequestBus.Event.SetPropertyOverride(self.entityId, assignmentId, propertyName, textureAssetId);
    end
end

function FindMaterialAssignmentTest:UpdateProperties()
    Debug.Log("Overriding properties...")
    for index = 1, #self.assignmentIds do
        local id = self.assignmentIds[index]
        if (id ~= nil) then
            self:UpdateFactor(id)
            self:UpdateTexture(id)
        end
    end
end

function FindMaterialAssignmentTest:ClearProperties()
    Debug.Log("Clearing properties...")
    MaterialComponentRequestBus.Event.ClearAllPropertyOverrides(self.entityId);
end

function lerpColor(color, lerpDir, deltaTime)
    local lerpSpeed = 0.5
    color.r = color.r + deltaTime * lerpDir[1] * lerpSpeed
    if color.r > 1.0 then
        color.r = 1.0
        lerpDir[1] = -1.0
    elseif color.r < 0 then
        color.r = 0
        lerpDir[1] = 1.0
    end

    color.g = color.g + deltaTime * lerpDir[2] * lerpSpeed
    if color.g > 1.0 then
        color.g = 1.0
        lerpDir[2] = -1.0
    elseif color.g < 0 then
        color.g = 0
        lerpDir[2] = 1.0
    end

    color.b = color.b + deltaTime * lerpDir[3] * lerpSpeed
    if color.b > 1.0 then
        color.b = 1.0
        lerpDir[3] = -1.0
    elseif color.b < 0 then
        color.b = 0
        lerpDir[3] = 1.0
    end
end

function FindMaterialAssignmentTest:lerpColors(deltaTime)
    for index = 1, #self.assignmentIds do
        local id = self.assignmentIds[index]
        if (id ~= nil) then
            lerpColor(self.colors[index], self.lerpDirs[index], deltaTime)
            self:UpdateColor(id, self.colors[index])
        end
    end
end

function FindMaterialAssignmentTest:OnTick(deltaTime, timePoint)


    if(nil == self.assignmentIds) then
    
        local originalAssignments = MaterialComponentRequestBus.Event.GetOriginalMaterialAssignments(self.entityId)
        if(nil == originalAssignments or #originalAssignments <= 1) then -- There is always 1 entry for the default assignment; a loaded model will have at least 2 assignments
            return
        end
        
        self.assignmentIds =
        {
            MaterialComponentRequestBus.Event.FindMaterialAssignmentId(self.entityId, -1, self.Properties.MaterialSlotFilter),
        }
    
        for index = 1, #self.assignmentIds do
            local id = self.assignmentIds[index]
            if (id ~= nil) then
                self.colors[index] = randomColor()
                self.lerpDirs[index] = randomDir()
            end
        end
    end
    
    self.timer = self.timer + deltaTime
    self.totalTime = self.totalTime + deltaTime
    self:lerpColors(deltaTime)
    
    if (self.timer > self.timeUpdate and self.totalTime < self.totalTimeMax) then
        self.timer = self.timer - self.timeUpdate
        self:UpdateProperties()
    elseif self.totalTime > self.totalTimeMax then
        self:ClearProperties()
        self.tickBusHandler:Disconnect(self);
    end

end

return FindMaterialAssignmentTest
