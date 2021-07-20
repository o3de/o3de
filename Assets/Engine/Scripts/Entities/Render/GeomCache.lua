----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
Script.ReloadScript("scripts/Utils/EntityUtils.lua")

GeomCache =
{
    Properties = {
        geomcacheFile = "EngineAssets/GeomCaches/defaultGeomCache.cax",
        bPlaying = 0,
        fStartTime = 0,
        bLooping = 0,
        objectStandIn = "",
        materialStandInMaterial = "",
        objectFirstFrameStandIn = "",
        materialFirstFrameStandInMaterial = "",
        objectLastFrameStandIn = "",
        materialLastFrameStandInMaterial = "",
        fStandInDistance = 0,
        fStreamInDistance = 0,        
        Physics = {
            bPhysicalize = 0,
        }        
    },

    Editor={
        Icon = "animobject.bmp",
        IconOnTop = 1,
    },
    
    bPlaying = 0,
    currentTime = 0,
    precacheTime = 0,
    bPrecachedOutputTriggered = false,
}

function GeomCache:OnLoad(table)  
    self.currentTime = table.currentTime;    
end

function GeomCache:OnSave(table)  
    table.currentTime = self.currentTime;
end

function GeomCache:OnSpawn()
    self.currentTime = self.Properties.fStartTime;
    self:SetFromProperties();
end

function GeomCache:OnReset()
    self.currentTime = self.Properties.fStartTime;
    self.bPrecachedOutputTriggered = true;
    self:SetFromProperties();    
end

function GeomCache:SetFromProperties()
    local Properties = self.Properties;
    
    if (Properties.geomcacheFile == "") then
        do return end;
    end    
    
    self:LoadGeomCache(0, Properties.geomcacheFile);    
    
    self.bPlaying = Properties.bPlaying;
    if (self.bPlaying == 0) then
        self.currentTime = Properties.fStartTime;
    end    
    
    self:SetGeomCachePlaybackTime(self.currentTime);
    self:SetGeomCacheParams(Properties.bLooping, Properties.objectStandIn, Properties.materialStandInMaterial, Properties.objectFirstFrameStandIn, 
        Properties.materialFirstFrameStandInMaterial, Properties.objectLastFrameStandIn, Properties.materialLastFrameStandInMaterial, 
        Properties.fStandInDistance, Properties.fStreamInDistance);
    self:SetGeomCacheStreaming(false, 0);

    if (Properties.Physics.bPhysicalize == 1) then
        local tempPhysParams = EntityCommon.TempPhysParams;
        self:Physicalize(0, PE_ARTICULATED, tempPhysParams);
    end
    
    self:Activate(1);
end

function GeomCache:PhysicalizeThis()
    local Physics = self.Properties.Physics;
    EntityCommon.PhysicalizeRigid(self, 0, Physics, false);
end

function GeomCache:OnUpdate(dt)
    if (self.bPlaying == 1) then
        self:SetGeomCachePlaybackTime(self.currentTime);
    end

    if (self:IsGeomCacheStreaming() and not self.bPrecachedOutputTriggered) then
        local precachedTime = self:GetGeomCachePrecachedTime();
        if (precachedTime >= self.precacheTime) then            
            self:ActivateOutput("Precached", true);
            self.bPrecachedOutputTriggered = true;
        end
    end

    if (self.bPlaying == 1) then
        self.currentTime = self.currentTime + dt;
    end        
end

function GeomCache:OnPropertyChange()
    self:SetFromProperties();
end

function GeomCache:Event_Start(sender, val)
    self.bPlaying = 1;
end

function GeomCache:Event_Stop(sender, value)
    self.bPlaying = 0;
end

function GeomCache:Event_SetTime(sender, value)
    self.currentTime = value;
end

function GeomCache:Event_StartStreaming(sender, value)
    self.bPrecachedOutputTriggered = false;
    self:SetGeomCacheStreaming(true, self.currentTime);
end

function GeomCache:Event_StopStreaming(sender, value)
    self:SetGeomCacheStreaming(false, 0);
end

function GeomCache:Event_PrecacheTime(sender, value)
    self.precacheTime = value;
end

function GeomCache:Event_Hide(sender, value)
    self:Hide(1);
end

function GeomCache:Event_Unhide(sender, value)
    self:Hide(0);
end

function GeomCache:Event_StopDrawing(sender, value)
    self:SetGeomCacheDrawing(false);
end

function GeomCache:Event_StartDrawing(sender, value)
    self:SetGeomCacheDrawing(true);
end

GeomCache.FlowEvents =
{
    Inputs =
    {
        Start = { GeomCache.Event_Start, "any" },
        Stop = { GeomCache.Event_Stop, "any" },
        SetTime = { GeomCache.Event_SetTime, "float" },
        StartStreaming = { GeomCache.Event_StartStreaming, "any" },
        StopStreaming = { GeomCache.Event_StopStreaming, "any" },
        PrecacheTime = { GeomCache.Event_PrecacheTime, "float" },
        Hide = { GeomCache.Event_Hide, "any" },
        Unhide = { GeomCache.Event_Unhide, "any" },
        StopDrawing = { GeomCache.Event_StopDrawing, "any" },
        StartDrawing = { GeomCache.Event_StartDrawing, "any" },
    },
    Outputs =
    {
        Precached = "bool",
    },
}
