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

local OutputProfileData = 
{
    Properties = 
    {
        FrameDelayCount = 100,
        FrameCaptureCount = 100,
        ProfileName = "LevelFrameTiming",
        QuitOnComplete = true,
    },
    frameCount = 0,
    frameDelayCount = 0,
    frameCaptureCount = 0,
    captureCount = 0,
    quitOnComplete = false,
    profileName = "UninitializedName",
    outputFolder = "UninitializedPath",
    cpuTimingsOutputPath = "UninitializedPath",
    captureInProgress = false,
    active = false,
};

local FrameTimeRecordingActiveRegistryKey <const> = "/O3DE/Performance/FrameTimeRecording/Activate"
local FrameDelayCountRegistryKey <const> = "/O3DE/Performance/FrameTimeRecording/DelayCount"
local FrameCaptureCountRegistryKey <const> = "/O3DE/Performance/FrameTimeRecording/CaptureCount"
local ProfileNameRegistryKey <const> = "/O3DE/Performance/FrameTimeRecording/ProfileName"
local QuitOnCompleteRegistryKey <const> = "/O3DE/Performance/FrameTimeRecording/QuitOnComplete"
local SourceProjectUserPathRegistryKey <const> = "/O3DE/Runtime/FilePaths/SourceProjectUserPath"
local ConsoleCommandQuitRegistryKey <const> = "/Amazon/AzCore/Runtime/ConsoleCommands/quit"

function OutputProfileData:TryDisconnect()
    self.profileCaptureNotificationHandler:Disconnect()
    self.tickHandler:Disconnect()
end

function OutputProfileData:TryQuitOnComplete()
    if (self.quitOnComplete) then
        g_SettingsRegistry:SetString(ConsoleCommandQuitRegistryKey, "")
    end
end

function OutputProfileData:TryCapture()
    self.captureInProgress = true
    self.cpuTimingsOutputPath = tostring(self.outputFolder) .. '/cpu_frame' .. tostring(self.captureCount) .. '_time.json'
    ProfilingCaptureRequestBus.Broadcast.CaptureCpuFrameTime(self.cpuTimingsOutputPath)
end

function OutputProfileData:CaptureFinished()
    self.captureInProgress = false
end


function OutputProfileData:OnActivate()
    if (g_SettingsRegistry:IsValid()) then
        local quitOnCompleteValue = g_SettingsRegistry:GetBool(QuitOnCompleteRegistryKey)
        self.quitOnComplete = quitOnCompleteValue:value_or(self.Properties.QuitOnComplete)

        local frameTimeRecordingActivateValue = g_SettingsRegistry:GetBool(FrameTimeRecordingActiveRegistryKey)
        if (not frameTimeRecordingActivateValue:has_value() or not frameTimeRecordingActivateValue:value()) then
            Debug:Log("OutputProfileData:OnActivate - Missing registry setting to activate frame time recording, aborting data collection")
            return
        end

        -- get path to user folder
        local pathToUserFolder = "InvalidPath/"
        local settingsRegistryResult =  g_SettingsRegistry:GetString(SourceProjectUserPathRegistryKey)
        if (settingsRegistryResult:has_value()) then
            pathToUserFolder = settingsRegistryResult:value()
        else
            Debug:Log("OutputProfileData:OnActivate - Unable to resolve the SourceProjectUserPath, aborting data collection")
            self:TryQuitOnComplete()
            return
        end

        -- get any registry property overrides
        local frameDelayCountValue = g_SettingsRegistry:GetUInt(FrameDelayCountRegistryKey)
        self.frameDelayCount = frameDelayCountValue:value_or(self.Properties.FrameDelayCount)
        local frameCaptureCountValue = g_SettingsRegistry:GetUInt(FrameCaptureCountRegistryKey)
        self.frameCaptureCount = frameCaptureCountValue:value_or(self.Properties.FrameCaptureCount)
        local profileNameValue = g_SettingsRegistry:GetString(ProfileNameRegistryKey)
        self.profileName = profileNameValue:value_or(self.Properties.ProfileName)

        -- generate final output path string
        self.outputFolder = tostring(pathToUserFolder) .. "/Scripts/PerformanceBenchmarks/" .. tostring(self.profileName)

        -- register for ebus callbacks
        self.tickHandler = TickBus.Connect(self)
        self.profileCaptureNotificationHandler = ProfilingCaptureNotificationBus.Connect(self)

        -- output test metadata
        ProfilingCaptureRequestBus.Broadcast.CaptureBenchmarkMetadata(
            self.profileName, tostring(self.outputFolder) .. "/benchmark_metadata.json")
    end
end

function OutputProfileData:OnTick()
    if (not self.active) then -- wait for benchmark metadata to get written before starting
        return
    end

    self.frameCount = self.frameCount + 1
    if (self.frameCount <= self.frameDelayCount) then
        return
    end

    if (self.captureCount < self.frameCaptureCount and not self.captureInProgress) then
        self.captureCount = self.captureCount + 1
        self:TryCapture()
    elseif (self.captureCount >= self.frameCaptureCount and not self.captureInProgress) then
        Debug:Log("OutputProfileData complete")
        self:TryDisconnect()
        self:TryQuitOnComplete()
    end
end

function OutputProfileData:OnCaptureCpuFrameTimeFinished(successful, capture_output_path)
    if (self.captureInProgress and self.cpuTimingsOutputPath == capture_output_path) then
        self:CaptureFinished()
    end
end

function OutputProfileData:OnCaptureBenchmarkMetadataFinished(successful, info)
    if (not successful) then
        Debug:Log("OutputProfileData - Failed to capture benchmark metadata, aborting data collection")
        -- force profile to end asap without trying to record
        self.frameCount = self.frameDelayCount
        self.captureCount = self.FrameCaptureCount
        self:TryDisconnect()
        self:TryQuitOnComplete()
    else
        self.active = true
    end
end


function OutputProfileData:OnDeactivate()
    self:TryDisconnect()
end

return OutputProfileData
