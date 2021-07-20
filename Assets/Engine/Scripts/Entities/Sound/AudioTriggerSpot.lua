----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
Script.ReloadScript("scripts/Entities/Sound/Shared/AudioUtils.lua");

AudioTriggerSpot = {
    type = "AudioTriggerSpot",

    Editor = {
        Model = "Editor/Objects/Sound.cgf",
        Icon = "Sound.bmp",
    },
    
    Properties = {
        bEnabled = true,
        audioTriggerPlayTriggerName = "",
        audioTriggerStopTriggerName = "",
        bSerializePlayState = true, -- Determines if execution after de-serialization is needed.
        eiSoundObstructionType = AUDIO_OBSTRUCTION_TYPE_IGNORE,
        bPlayOnX = false,
        bPlayOnY = false,
        bPlayOnZ = false,
        fRadiusRandom = 10.0,
        bPlayRandom = false,
        fMinDelay = 1,
        fMaxDelay = 2,
    },
    
    hOnTriggerID = nil,
    hOffTriggerID = nil,
    hCurrentOnTriggerID = nil,
    hCurrentOffTriggerID = nil, -- only used in OnPropertyChange()
    tObstructionType = {},
    
    bIsHidden = false,
    bIsPlaying = false,
    bHasMoved = false,
    bOriginalEnabled = true,
}

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_LookupTriggerIDs()
    self.hOnTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerPlayTriggerName);
    self.hOffTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerStopTriggerName);
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_LookupObstructionSwitchIDs()
    -- cache the obstruction switch and state IDs
    self.tObstructionType = AudioUtils.LookupObstructionSwitchAndStates();
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_SetObstruction()
    local nStateIdx = self.Properties.eiSoundObstructionType + 1;
    self:SetAudioObstructionCalcType(self.Properties.eiSoundObstructionType, self:GetDefaultAuxAudioProxyID());
    if ((self.tObstructionType.hSwitchID ~= nil) and (self.tObstructionType.tStateIDs[nStateIdx] ~= nil)) then
        self:SetAudioSwitchState(self.tObstructionType.hSwitchID, self.tObstructionType.tStateIDs[nStateIdx], self:GetDefaultAuxAudioProxyID());
    end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_GenerateOffset()
    local offset = {x=0,y=0,z=0}
    local len = 0
    
    if (self.Properties.bPlayOnX) then
        offset.x=randomF(-1,1);
    end
    if (self.Properties.bPlayOnY) then
        offset.y=randomF(-1,1);
    end
    if (self.Properties.bPlayOnZ) then
        offset.z=randomF(-1,1);
    end
    
    NormalizeVector(offset);
    ScaleVectorInPlace(offset, randomF(0,self.Properties.fRadiusRandom));
    
    return offset;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnSpawn()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnSave(save)
    save.Properties = self.Properties;
    save.bIsPlaying = self.bIsPlaying;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnLoad(load)
    self.Properties = load.Properties;
    self.bIsPlaying = load.bIsPlaying;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnPostLoad()
    self:_SetObstruction();
    self:SetCurrentAudioEnvironments();
    
    if (self.bIsPlaying and self.Properties.bSerializePlayState) then
        self.bIsPlaying = false;
        self:Play();
    else
        self.bIsPlaying = false;
    end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_Init()
    self.bIsPlaying = false;
    self:SetAudioProxyOffset(g_Vectors.v000, self:GetDefaultAuxAudioProxyID());
    self:NetPresent(0);
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnPropertyChange()
    if (self.Properties.eiSoundObstructionType < AUDIO_OBSTRUCTION_TYPE_IGNORE) then
        self.Properties.eiSoundObstructionType = AUDIO_OBSTRUCTION_TYPE_IGNORE;
    elseif (self.Properties.eiSoundObstructionType > AUDIO_OBSTRUCTION_TYPE_MULTI) then
        self.Properties.eiSoundObstructionType = AUDIO_OBSTRUCTION_TYPE_MULTI;
    end
    
    self:_LookupTriggerIDs();
    self:_SetObstruction();
    self:SetCurrentAudioEnvironments();
    self:SetAudioProxyOffset(g_Vectors.v000, self:GetDefaultAuxAudioProxyID());
    
    if ((self.bIsPlaying) and (self.hCurrentOnTriggerID ~= self.hOnTriggerID)) then
        -- Stop a possibly playing instance if the on-trigger changed!
        self:StopAudioTrigger(self.hCurrentOnTriggerID, self:GetDefaultAuxAudioProxyID());
        self.hCurrentOnTriggerID = self.hOnTriggerID;
        self.bIsPlaying = false;
        self.bHasMoved = false;
        self:KillTimer(0);
    end
    
    if (not self.bIsPlaying) then
        -- Try to play, if disabled, hidden or invalid on-trigger Play() will fail!
        self:Play();
    end
    
    if (not self.Properties.bEnabled and ((self.bOriginalEnabled) or (self.hCurrentOffTriggerID ~= self.hOffTriggerID))) then
        self.hCurrentOffTriggerID = self.hOffTriggerID;
        self:Stop(); -- stop if disabled, either stops running StartTrigger or executes StopTrigger!
    end
    
    self.bOriginalEnabled = self.Properties.bEnabled;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnReset(bToGame)
    if (bToGame) then
        -- store the entity's "bEnabled" property's value so we can adjust back to it if changed over the course of the game
        self.bOriginalEnabled = self.Properties.bEnabled;
        
        -- re-execute this ATS once upon entering game mode
        self:Stop();
        self:Play();
    else
        self.Properties.bEnabled = self.bOriginalEnabled;
    end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnTransformFromEditorDone()
    self:SetCurrentAudioEnvironments();
end

----------------------------------------------------------------------------------------
AudioTriggerSpot["Server"] = {
    OnInit = function (self)
        self:_Init();
    end,
    OnShutDown = function (self)
    end,
}

----------------------------------------------------------------------------------------
AudioTriggerSpot["Client"] = {
    ----------------------------------------------------------------------------------------
    OnInit = function(self)
        self:_Init();
        self:_LookupTriggerIDs();
        self:_LookupObstructionSwitchIDs();
        self:_SetObstruction();
        self:SetCurrentAudioEnvironments();
        self:Play();
    end,
    
    ----------------------------------------------------------------------------------------
    OnShutDown = function(self)
        self:StopAll();
    end,
    
    ----------------------------------------------------------------------------------------
    OnSoundDone = function(self, hTriggerID)
        if (self.hOnTriggerID == hTriggerID) then
            self:ActivateOutput("Done", true);
        end
    end,
    
    ----------------------------------------------------------------------------------------
    OnTimer = function(self, timerid, msec)
        if (timerid == 0) then
            self:Play();
        end
    end,
    
    ----------------------------------------------------------------------------------------
    OnHidden = function(self)
        self:StopAll();
        self.bIsHidden = true;
    end,
 
    ----------------------------------------------------------------------------------------
    OnUnHidden = function(self)
        self.bIsHidden = false;
        self:Play();
    end,
    
    ----------------------------------------------------------------------------------------
    OnMove = function(self)
        self.bHasMoved = true;
    end,
}

----------------------------------------------------------------------------------------
function AudioTriggerSpot:Play()
    if ((self.hOnTriggerID ~= nil) and (self.Properties.bEnabled) and (not self.bIsHidden)) then
        local offset = self:_GenerateOffset();
        if (LengthSqVector(offset) > 0.00001) then  -- offset is longer than 1cm
            self:SetAudioProxyOffset(offset, self:GetDefaultAuxAudioProxyID());
            self:SetCurrentAudioEnvironments();
        elseif (self.bHasMoved) then
            self:SetCurrentAudioEnvironments();
        end
        
        self:ExecuteAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
        self.bIsPlaying = true;
        self.bHasMoved = false;
        self.hCurrentOnTriggerID = self.hOnTriggerID;
        
        if (self.Properties.bPlayRandom) then
            self:SetTimer(0, 1000 * randomF(self.Properties.fMinDelay, self.Properties.fMaxDelay));
        end
    end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:Stop()
    if (not self.bIsHidden) then
        -- Cannot check against "self.bIsPlaying" otherwise we won't execute the StopTrigger if there's no StartTrigger set!
        if (self.hOffTriggerID ~= nil) then
            local offset = self:_GenerateOffset();
            if (LengthSqVector(offset) > 0.00001) then-- offset is longer than 1cm
                self:SetAudioProxyOffset(offset, self:GetDefaultAuxAudioProxyID());
                self:SetCurrentAudioEnvironments();
            elseif (self.bHasMoved) then
                self:SetCurrentAudioEnvironments();
            end
        
            self:ExecuteAudioTrigger(self.hOffTriggerID, self:GetDefaultAuxAudioProxyID());
        elseif (self.hOnTriggerID ~= nil) then
            self:StopAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
        end
    end
        
    self.bIsPlaying = false;
    self.bHasMoved = false;
    self:KillTimer(0);
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:StopAll()
    if (self.hOnTriggerID ~= nil) then
        self:StopAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
    end
    if (self.hOffTriggerID ~= nil) then
        self:StopAudioTrigger(self.hOffTriggerID, self:GetDefaultAuxAudioProxyID());
    end
    self.bIsPlaying = false;
    self:KillTimer(0);
end

------------------------------------------------------------------------------------------------------
-- Event Handlers
------------------------------------------------------------------------------------------------------
function AudioTriggerSpot:Event_Enable(sender)
    if (not self.Properties.bEnabled) then
        self.Properties.bEnabled = true;
        self:Play();
    end
    --BroadcastEvent(self, "Enable");
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:Event_Disable(sender)
    self:Stop();
    self.Properties.bEnabled = false;
    --BroadcastEvent(self, "Disable");
end

----------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------
AudioTriggerSpot.FlowEvents =
{
    Inputs =
    {
        Enable = { AudioTriggerSpot.Event_Enable, "bool" },
        Disable = { AudioTriggerSpot.Event_Disable, "bool" },
    },
    
    Outputs =
    {
        Done = "bool",
    },
}


