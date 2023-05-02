----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------
function GetRequiredStringValue(valueKey, prettyName)
    value = g_SettingsRegistry:GetString(valueKey)
    if (not value:has_value()) then
        Print('FrameTime script missing ' .. tostring(prettyName) .. ' settings registry entry, ending script early')
        return false, nil
    end
    return true, value:value()
end

function GetOptionalUIntValue(valueKey, defaultValue)
    return g_SettingsRegistry:GetUInt(valueKey):value_or(defaultValue)
end
function GetOptionalStringValue(valueKey, defaultValue)
    return g_SettingsRegistry:GetString(valueKey):value_or(defaultValue)
end

-- required settings
local LevelPathRegistryKey <const> = "/O3DE/ScriptAutomation/ImageCapture/LevelPath"
local TestGroupNameRegistryKey <const> = "/O3DE/ScriptAutomation/ImageCapture/TestGroupName" -- used as part of capture filepath, no whitespace or other invalid characters
local ImageNameRegistryKey <const> = "/O3DE/ScriptAutomation/ImageCapture/ImageName"

-- optional settings
local ImageComparisonLevelRegistryKey <const> = "/O3DE/ScriptAutomation/ImageCapture/ImageComparisonLevel"

-- check for SettingsRegistry values that must exist
succeeded, levelPath = GetRequiredStringValue(LevelPathRegistryKey, "Image Capture Level Path")
if (not succeeded) then return end
succeeded, testGroupName = GetRequiredStringValue(TestGroupNameRegistryKey, "Test Group Name")
if (not succeeded) then return end
succeeded, imageName = GetRequiredStringValue(ImageNameRegistryKey, "Image Capture Name")
if (not succeeded) then return end
local imageComparisonLevel = GetOptionalStringValue(ImageComparisonLevelRegistryKey, "Level A") -- default to most strict comparison


RunScript("@gemroot:ScriptAutomation@/Assets/AutomationScripts/GenericImageComparisonTestEnvironment.lua")
captureName = testGroupName .. "/" .. imageName

IdleFrames(3) -- tick 3 frames to allow tick delta to settle

ExecuteConsoleCommand("r_displayInfo=0")
LoadLevel(levelPath) -- waits for the engine to say the level is finished loading

IdleSeconds(2) -- Wait for assets to finish loading.

Print("Saving screenshots to " .. ResolvePath(g_screenshotOutputFolder .. "/" .. g_testEnv .. "/" .. testGroupName))

CaptureScreenshot(captureName)

CompareScreenshotToBaseline(testGroupName .. "/" .. g_testEnv, imageComparisonLevel, captureName, g_imperceptibleImageDiffLevel)
