----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
-- audio area ambience entity - to be attached to an area
-- used for convenient implementation of area based audio ambiences

AudioAreaRandom = {
    type = "AudioAreaRandom",
    
    Editor = {
        Model = "Editor/Objects/Sound.cgf",
        Icon = "AudioAreaRandom.bmp",
    },
    
    Properties = {
        bEnabled = true,
        bMoveWithEntity = false,
        audioTriggerPlayTrigger = "",
        audioTriggerStopTrigger = "",
        audioRTPCRtpc = "",
        eiSoundObstructionType = AUDIO_OBSTRUCTION_TYPE_IGNORE,
        fRtpcDistance = 5.0,
        fRadiusRandom = 10.0,
        fMinDelay = 1,
        fMaxDelay = 2,
    },
  
    fFadeValue = 0.0,
    nState = 0, -- 0 = far, 1 = near, 2 = inside
    hOnTriggerID = nil,
    hOffTriggerID = nil,
    hCurrentOnTriggerID = nil,
    hCurrentOffTriggerID = nil, -- only used in OnPropertyChange()
    hRtpcID = nil,
    tObstructionType = {},
    bIsHidden = false,
    bIsPlaying = false,
    bHasMoved = false,
    bOriginalEnabled = true,
}

----------------------------------------------------------------------------------------
function AudioAreaRandom:_LookupControlIDs()
    self.hOnTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerPlayTrigger);    
    self.hOffTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerStopTrigger);
    self.hRtpcID = AudioUtils.LookupRtpcID(self.Properties.audioRTPCRtpc);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_LookupObstructionSwitchIDs()
    -- cache the obstruction switch and state IDs
    self.tObstructionType = AudioUtils.LookupObstructionSwitchAndStates();
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_SetObstruction()
    local nStateIdx = self.Properties.eiSoundObstructionType + 1;
    self:SetAudioObstructionCalcType(self.Properties.eiSoundObstructionType, self:GetDefaultAuxAudioProxyID());
    if ((self.tObstructionType.hSwitchID ~= nil) and (self.tObstructionType.tStateIDs[nStateIdx] ~= nil)) then
        self:SetAudioSwitchState(self.tObstructionType.hSwitchID, self.tObstructionType.tStateIDs[nStateIdx], self:GetDefaultAuxAudioProxyID());
    end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_UpdateParameters()
    self:SetFadeDistance(self.Properties.fRtpcDistance);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_UpdateRtpc()
    if (self.hRtpcID ~= nil) then
        self:SetAudioRtpcValue(self.hRtpcID, self.fFadeValue, self:GetDefaultAuxAudioProxyID());
    end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_GenerateOffset()
    local offset = {x = 0, y = 0, z = 0}
    offset.x = randomF(-1, 1);
    offset.y = randomF(-1, 1);
    NormalizeVector(offset);
    ScaleVectorInPlace(offset, randomF(0, self.Properties.fRadiusRandom));

    return offset;
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnSpawn()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnLoad(load)
    self.Properties = load.Properties;
    self.fFadeValue = load.fFadeValue;
    self.nState = 0; -- We start out being far, in a subsequent update we will determine our actual state!

    self:_SetObstruction();
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnPostLoad()
    self:_UpdateParameters();
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnSave(save)
    save.Properties = self.Properties;
    save.fFadeValue = self.fFadeValue;
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnPropertyChange()
    if (self.Properties.eiSoundObstructionType < AUDIO_OBSTRUCTION_TYPE_IGNORE) then
        self.Properties.eiSoundObstructionType = AUDIO_OBSTRUCTION_TYPE_IGNORE;
    elseif (self.Properties.eiSoundObstructionType > AUDIO_OBSTRUCTION_TYPE_MULTI) then
        self.Properties.eiSoundObstructionType = AUDIO_OBSTRUCTION_TYPE_MULTI;
    end
    
    self:_LookupControlIDs();
    self:_UpdateParameters();
    self:_SetObstruction();
    self:ResetAudioRtpcValues(self:GetDefaultAuxAudioProxyID());
    self:SetCurrentAudioEnvironments();
    self:SetAudioProxyOffset(g_Vectors.v000, self:GetDefaultAuxAudioProxyID());
    self:AuxAudioProxiesMoveWithEntity(self.Properties.bMoveWithEntity);
    
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
function AudioAreaRandom:OnReset(bToGame)
    if (bToGame) then
        -- store the entity's "bEnabled" property's value so we can adjust back to it if changed over the course of the game
        self.bOriginalEnabled = self.Properties.bEnabled;
        
        -- re-execute this AAR once upon entering game mode
        self:Stop();
        self:Play();
    else
        self.Properties.bEnabled = self.bOriginalEnabled;
    end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:Play()
    if ((self.Properties.bEnabled) and (not self.bIsHidden) and ((self.nState == 1) or (self.nState == 2))) then
        if (self.hOnTriggerID ~= nil) then
            local offset = self:_GenerateOffset();
            if (LengthSqVector(offset) > 0.00001) then-- offset is longer than 1cm
                self:SetAudioProxyOffset(offset, self:GetDefaultAuxAudioProxyID());
                self:SetCurrentAudioEnvironments();
            elseif (self.bHasMoved) then
                self:SetCurrentAudioEnvironments();
            end
            
            self:ExecuteAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
            self.bIsPlaying = true;
            self.bHasMoved = false;
            self.hCurrentOnTriggerID = self.hOnTriggerID;
            
            self:SetTimer(0, 1000 * randomF(self.Properties.fMinDelay, self.Properties.fMaxDelay));
        end
    end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:Stop()
    if ((self.Properties.bEnabled) and (not self.bIsHidden) and ((self.nState == 1) or (self.nState == 2))) then
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
function AudioAreaRandom:StopAll()
    if (self.hOnTriggerID ~= nil) then
        self:StopAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
    end
    if (self.hOffTriggerID ~= nil) then
        self:StopAudioTrigger(self.hOffTriggerID, self:GetDefaultAuxAudioProxyID());
    end
    self.bIsPlaying = false;
    self:KillTimer(0);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:CliSrv_OnInit()
    self.nState = 0;
    self.fFadeValue = 0.0;
    self:SetFlags(ENTITY_FLAG_VOLUME_SOUND, 0);
    self:_UpdateParameters();
    self.bIsPlaying = false;
    self:NetPresent(0);
    self:AuxAudioProxiesMoveWithEntity(self.Properties.bMoveWithEntity);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:UpdateFadeValue(player, fFade, fDistSq)
    if (not(self.Properties.bEnabled) or (fFade == 0.0 and fDistSq == 0.0)) then
        self.fFadeValue = 0.0;
        self:_UpdateRtpc();
        do return end;
    end
    
    if (self.Properties.fRtpcDistance > 0.0) then
        if (self.nState == 2) then
            if (self.fFadeValue ~= fFade) then
                self.fFadeValue = math.abs(fFade);
                self:_UpdateRtpc();
            end
        else
            local fLocalFade = 1.0 - (math.sqrt(fDistSq) / self.Properties.fRtpcDistance);
            self.fFadeValue = math.max(0, fLocalFade);
            self:_UpdateRtpc();
        end
    end
end

----------------------------------------------------------------------------------------
AudioAreaRandom.Server = {
    OnInit = function(self)
        self:CliSrv_OnInit();
    end,

    OnShutDown = function(self)
    end,
}

----------------------------------------------------------------------------------------
AudioAreaRandom.Client = {
    OnInit = function(self)
        self:RegisterForAreaEvents(1);
        self:_LookupControlIDs();
        self:_LookupObstructionSwitchIDs();
        self:_SetObstruction();
        self:CliSrv_OnInit();
    end,

    OnShutDown = function(self)
        self:StopAll();
        self.nState = 0;
        self:RegisterForAreaEvents(0);
    end,
    
    OnHidden = function(self)
        self:StopAll();
        self.bIsHidden = true;
    end,

    OnUnHidden = function(self)
        self.bIsHidden = false;
        self:Play();
    end,

    OnAudioListenerEnterNearArea = function(self, player, nAreaID, fFade)
        if (self.nState == 0) then
            self.nState = 1;
            self:Play();
            self.fFadeValue = 0.0;
            self:_UpdateRtpc();
        end
    end,
    
    OnAudioListenerMoveNearArea = function(self, player, areaId, fFade, fDistsq)
        self.nState = 1;
        self:UpdateFadeValue(player, fFade, fDistsq);
    end,
    
    OnAudioListenerEnterArea = function(self, player, areaId, fFade)
        if (self.nState == 0) then
            -- possible if the listener is teleported or gets spawned inside the area
            -- technically, the listener enters the Near Area and the Inside Area at the same time
            self.nState = 2;
            self:Play();
        else
            self.nState = 2;
        end
        
        self.fFadeValue = 1.0;
        self:_UpdateRtpc();
    end,
    
    OnAudioListenerProceedFadeArea = function(self, player, areaId, fExternalFade)
        -- fExternalFade holds the fade value which was calculated by an inner, higher priority area
        -- in the AreaManager to fade out the outer sound dependent on the largest fade distance of all attached entities
        if (fExternalFade > 0.0) then
            self.nState = 2;
            self:UpdateFadeValue(player, fExternalFade, 0.0);
        else
            self:UpdateFadeValue(player, 0.0, 0.0);
        end
    end,
    
    OnAudioListenerLeaveArea = function(self, player, nAreaID, fFade)
        self.nState = 1;
    end,
    
    OnAudioListenerLeaveNearArea = function(self, player, nAreaID, fFade)
        self:Stop();
        self.nState = 0;
        self.fFadeValue = 0.0;
        self:_UpdateRtpc();
    end,
    
    OnUnBindThis = function(self)
        self.nState = 0;
    end,
    
    OnTimer = function(self, timerid, msec)
        if (timerid == 0) then
            self:Play();
        end
    end,
    
    OnMove = function(self)
        self.bHasMoved = true;
    end,
}

----------------------------------------------------------------------------------------
-- Event Handlers
----------------------------------------------------------------------------------------
function AudioAreaRandom:Event_Enable(sender)
    self.Properties.bEnabled = true;
    self:OnPropertyChange();
end

function AudioAreaRandom:Event_Disable(sender)
    self.Properties.bEnabled = false;
    self:OnPropertyChange();
end

AudioAreaRandom.FlowEvents =
{
    Inputs =
    {
        Enable = { AudioAreaRandom.Event_Enable, "bool" },
        Disable = { AudioAreaRandom.Event_Disable, "bool" },
    },
}
