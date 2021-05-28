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
AudioUtils = {
    -- these names should exactly match those defined in ATLEntities.cpp and in libs/gameaudio/wwise/default_controls.xml
    sObstructionCalcSwitchName = "ObstructionOcclusionCalculationType",
    sObstructionStateNames = {"Ignore", "SingleRay", "MultiRay"},
}

----------------------------------------------------------------------------------------
function AudioUtils.LookupTriggerID(sTriggerName)
    local hTriggerID = nil;
    
    if ((sTriggerName ~= nil) and (sTriggerName ~= "")) then
        hTriggerID = Sound.GetAudioTriggerID(sTriggerName);
    end
    
    return hTriggerID;
end

----------------------------------------------------------------------------------------
function AudioUtils.LookupRtpcID(sRtpcName)
    local hRtpcID = nil;
    
    if ((sRtpcName ~= nil) and (sRtpcName ~= "")) then
        hRtpcID = Sound.GetAudioRtpcID(sRtpcName);
    end
    
    return hRtpcID;
end

----------------------------------------------------------------------------------------
function AudioUtils.LookupSwitchID(sSwitchName)
    local hSwitchID = nil;
    
    if ((sSwitchName ~= nil) and (sSwitchName ~= "")) then
        hSwitchID = Sound.GetAudioSwitchID(sSwitchName);
    end

    return hSwitchID;
end

----------------------------------------------------------------------------------------
function AudioUtils.LookupSwitchStateIDs(hSwitchID, tStateNames)
    local tStateIDs = {};
    
    if ((hSwitchID ~= nil) and (tStateNames ~= nil)) then
        for i, name in ipairs(tStateNames) do
            tStateIDs[i] = Sound.GetAudioSwitchStateID(hSwitchID, name);
        end
    end
    
    return tStateIDs;
end

----------------------------------------------------------------------------------------
function AudioUtils.LookupAudioEnvironmentID(sEnvironmentName)
    local hEnvironmentID = nil;
    
    if ((sEnvironmentName ~= nil) and (sEnvironmentName ~= "")) then
        hEnvironmentID = Sound.GetAudioEnvironmentID(sEnvironmentName);
    end
    
    return hEnvironmentID;
end

----------------------------------------------------------------------------------------
function AudioUtils.LookupObstructionSwitchAndStates()
    local nSwitch = AudioUtils.LookupSwitchID(AudioUtils.sObstructionCalcSwitchName);
    local tStates = AudioUtils.LookupSwitchStateIDs(nSwitch, AudioUtils.sObstructionStateNames);
    
    return {hSwitchID = nSwitch, tStateIDs = tStates};
end
