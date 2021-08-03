----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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
