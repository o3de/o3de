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
        Print('GenricRenderScreenshotTest script missing ' .. tostring(prettyName) .. ' settings registry entry, ending script early')
        return false, nil
    end
    Print('GenricRenderScreenshotTest script found ' .. prettyName .. ' settings registry entry, ' .. value:value())
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
local CaptureCameraNameKey <const> = "/O3DE/ScriptAutomation/ImageCapture/CaptureCameraName"
local ImageComparisonLevelRegistryKey <const> = "/O3DE/ScriptAutomation/ImageCapture/ImageComparisonLevel"

-- check for SettingsRegistry values that must exist
succeeded, levelPath = GetRequiredStringValue(LevelPathRegistryKey, "Image Capture Level Path")
if (not succeeded) then return end
succeeded, testGroupName = GetRequiredStringValue(TestGroupNameRegistryKey, "Test Group Name")
if (not succeeded) then return end
succeeded, imageNameStr = GetRequiredStringValue(ImageNameRegistryKey, "Image Capture Name")
if (not succeeded) then return end
succeeded, imageComparisonLevelStr = GetRequiredStringValue(ImageComparisonLevelRegistryKey, "Image Comparison Level")
if (not succeeded) then return end
succeeded, cameraNameStr = GetRequiredStringValue(CaptureCameraNameKey, "Camera Entity Names")
if (not succeeded) then return end

splitterChar = ","
imageNames = SplitString(imageNameStr, splitterChar)
imageComparisonLevels = SplitString(imageComparisonLevelStr, splitterChar)
cameraNames = SplitString(cameraNameStr, splitterChar)

if (imageNames:Size() ~= imageComparisonLevels:Size() or imageNames:Size() ~= cameraNames:Size()) then
    Error("Invalid number of arguments received")
    return
end

RunScript("@gemroot:ScriptAutomation@/Assets/AutomationScripts/GenericImageComparisonTestEnvironment.lua")

IdleFrames(3) -- tick 3 frames to allow tick delta to settle

ExecuteConsoleCommand("r_displayInfo=0")
LoadLevel(levelPath) -- waits for the engine to say the level is finished loading

IdleSeconds(2) -- Wait for assets to finish loading.

for index=1, imageNames:Size() do
    SetCamera(cameraNames[index])
    captureName = testGroupName .. "/" .. imageNames[index]

    IdleFrames(10) -- Wait for camera transition to happen

    Print("Saving screenshots to " .. ResolvePath(g_screenshotOutputFolder .. "/" .. g_testEnv .. "/" .. testGroupName))

    CaptureScreenshot(captureName)

    CompareScreenshotToBaseline(testGroupName .. "/" .. g_testEnv, imageComparisonLevels[index], captureName, g_imperceptibleImageDiffLevel)
end