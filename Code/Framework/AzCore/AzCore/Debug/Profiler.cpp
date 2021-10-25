/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AZ::Debug
{
    AZ_CVAR(AZ::CVarFixedString, bg_profilerCaptureLocation, "@user@/Profiler", nullptr, ConsoleFunctorFlags::Null,
        "Specify where to save profiler capture data");

    AZStd::string GenerateOutputFile(const char* nameHint)
    {
        AZStd::string timeString;
        AZStd::to_string(timeString, AZStd::GetTimeNowSecond());

        AZ::CVarFixedString captureOutput = static_cast<AZ::CVarFixedString>(bg_profilerCaptureLocation);

        return AZStd::string::format("%s/capture_%s_%s.json", captureOutput.c_str(), nameHint, timeString.c_str());
    }

    void ProfilerCaptureFrame([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZStd::string captureFile = GenerateOutputFile("single");
        AZLOG_INFO("Setting capture file to %s", captureFile.c_str());
        AZ::Debug::ProfilerRequestBus::Broadcast(&AZ::Debug::ProfilerRequestBus::Events::CaptureFrame, captureFile);
    }
    AZ_CONSOLEFREEFUNC(ProfilerCaptureFrame, AZ::ConsoleFunctorFlags::DontReplicate, "Capture a single frame of profiling data");

    void ProfilerStartCapture([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZStd::string captureFile = GenerateOutputFile("multi");
        AZLOG_INFO("Setting capture file to %s", captureFile.c_str());
        ProfilerRequestBus::Broadcast(&ProfilerRequestBus::Events::StartCapture, captureFile);
    }
    AZ_CONSOLEFREEFUNC(ProfilerStartCapture, AZ::ConsoleFunctorFlags::DontReplicate, "Start a multi-frame capture of profiling data");

    void ProfilerEndCapture([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ::Debug::ProfilerRequestBus::Broadcast(&AZ::Debug::ProfilerRequestBus::Events::EndCapture);
    }
    AZ_CONSOLEFREEFUNC(ProfilerEndCapture, AZ::ConsoleFunctorFlags::DontReplicate, "End and dump an in-progress continuous capture");
} // namespace AZ::Debug
