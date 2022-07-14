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
-- optional settings
local AssetLoadFrameIdleCountRegistryKey <const> = "/O3DE/ScriptAutomation/FrameTime/AssetFrameIdleCount"
local FrameIdleCountRegistryKey <const> = "/O3DE/ScriptAutomation/FrameTime/IdleCount"
local FrameCaptureCountRegistryKey <const> = "/O3DE/ScriptAutomation/FrameTime/CaptureCount"
local ViewportWidthRegistryKey <const> = "/O3DE/ScriptAutomation/FrameTime/ViewportWidth"
local ViewportHeightRegistryKey <const> = "/O3DE/ScriptAutomation/FrameTime/ViewportHeight"

-- required settings
local ProfileNameRegistryKey <const> = "/O3DE/ScriptAutomation/FrameTime/ProfileName"

-- default values
DEFAULT_ASSET_LOAD_FRAME_WAIT_COUNT = 100
DEFAULT_IDLE_COUNT = 100
DEFAULT_FRAME_COUNT = 100
DEFAULT_VIEWPORT_WIDTH = 800
DEFAULT_VIEWPORT_HEIGHT = 600

-- check for SettingsRegistry values that must exist
profileNameSR = SettingsRegistryGetString(ProfileNameRegistryKey)
if (not profileNameSR:has_value()) then
    Print('FrameTime script missing profileName settings registry entry, aborting')
    return;
end
profileName = profileNameSR:value()

-- get the output folder path
g_profileOutputFolder = GetProfilingOutputPath(true) .. "/" .. tostring(profileName)
Print('Saving screenshots to ' .. NormalizePath(g_profileOutputFolder))

-- read optional SettingsRegistry values
local assetLoadIdleFrameCount = SettingsRegistryGetUInt(AssetLoadFrameIdleCountRegistryKey):value_or(DEFAULT_ASSET_LOAD_FRAME_WAIT_COUNT)
local frameIdleCount = SettingsRegistryGetUInt(FrameIdleCountRegistryKey):value_or(DEFAULT_IDLE_COUNT)
local frameCaptureCount = SettingsRegistryGetUInt(FrameCaptureCountRegistryKey):value_or(DEFAULT_FRAME_COUNT)
local viewportWidth = SettingsRegistryGetUInt(ViewportWidthRegistryKey):value_or(DEFAULT_VIEWPORT_WIDTH)
local viewportHeight = SettingsRegistryGetUInt(ViewportHeightRegistryKey):value_or(DEFAULT_VIEWPORT_HEIGHT)


-- Begin script execution
ResizeViewport(viewportWidth, viewportHeight)
ExecuteConsoleCommand("r_displayInfo=0")

IdleFrames(assetLoadIdleFrameCount) -- wait for assets to load into the level

CaptureBenchmarkMetadata(tostring(profileName), g_profileOutputFolder .. '/benchmark_metadata.json')
Print('Idling for ' .. tostring(frameIdleCount) .. ' frames..')
IdleFrames(frameIdleCount)
Print('Capturing timestamps for ' .. tostring(frameCaptureCount) .. ' frames...')
for i = 1,frameCaptureCount do
    cpu_timings = g_profileOutputFolder .. '/cpu_frame' .. tostring(i) .. '_time.json'
    CaptureCpuFrameTime(cpu_timings)
end
Print('Capturing complete.')
