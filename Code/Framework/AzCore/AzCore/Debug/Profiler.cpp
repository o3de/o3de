/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/Debug/Profiler_Platform.inl>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/time.h>

namespace AZ::Debug
{
    AZStd::optional<Profiler*> ProfileScope::m_cachedProfiler;

    AZStd::string GenerateOutputFile(const char* nameHint)
    {
        AZ::IO::FixedMaxPathString captureOutput = GetProfilerCaptureLocation();
        return AZStd::string::format("%s/capture_%s_%lld.json", captureOutput.c_str(), nameHint, AZStd::GetTimeNowSecond());
    }

    void ProfilerCaptureFrame([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        if (auto profilerSystem = ProfilerSystemInterface::Get(); profilerSystem)
        {
            AZStd::string captureFile = GenerateOutputFile("single");
            AZLOG_INFO("Setting capture file to %s", captureFile.c_str());
            profilerSystem->CaptureFrame(captureFile);
        }
    }
    AZ_CONSOLEFREEFUNC(ProfilerCaptureFrame, AZ::ConsoleFunctorFlags::DontReplicate, "Capture a single frame of profiling data");

    void ProfilerStartCapture([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        if (auto profilerSystem = ProfilerSystemInterface::Get(); profilerSystem)
        {
            AZStd::string captureFile = GenerateOutputFile("multi");
            AZLOG_INFO("Setting capture file to %s", captureFile.c_str());
            profilerSystem->StartCapture(AZStd::move(captureFile));
        }
    }
    AZ_CONSOLEFREEFUNC(ProfilerStartCapture, AZ::ConsoleFunctorFlags::DontReplicate, "Start a multi-frame capture of profiling data");

    void ProfilerEndCapture([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        if (auto profilerSystem = ProfilerSystemInterface::Get(); profilerSystem)
        {
            profilerSystem->EndCapture();
        }
    }
    AZ_CONSOLEFREEFUNC(ProfilerEndCapture, AZ::ConsoleFunctorFlags::DontReplicate, "End and dump an in-progress continuous capture");

    AZ::IO::FixedMaxPathString GetProfilerCaptureLocation()
    {
        AZ::IO::FixedMaxPathString captureOutput;
        if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
        {
            settingsRegistry->Get(captureOutput, RegistryKey_ProfilerCaptureLocation);
        }

        if (captureOutput.empty())
        {
            captureOutput = ProfilerCaptureLocationFallback;
        }

        return captureOutput;
    }

    void ProfileScope::BeginRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, ...)
    {
#if !defined(AZ_RELEASE_BUILD)
        if (budget)
        {
            va_list args;
            va_start(args, eventName);
            Platform::BeginProfileRegion(budget, eventName, args);

            budget->BeginProfileRegion();

            // Initialize the cached pointer with the current handler or nullptr if no handlers are registered.
            // We do it here because Interface::Get will do a full mutex lock if no handlers are registered
            // causing big performance hit.
            if (!m_cachedProfiler.has_value())
            {
                m_cachedProfiler = AZ::Interface<Profiler>::Get();
            }

            if (m_cachedProfiler.value())
            {
                m_cachedProfiler.value()->BeginRegion(budget, eventName, args);
            }
        }
#endif // !defined(AZ_RELEASE_BUILD)
    }

    void ProfileScope::EndRegion([[maybe_unused]] Budget* budget)
    {
#if !defined(AZ_RELEASE_BUILD)
        if (budget)
        {
            budget->EndProfileRegion();

            if (m_cachedProfiler.value())
            {
                m_cachedProfiler.value()->EndRegion(budget);
            }

            Platform::EndProfileRegion(budget);
        }
#endif // !defined(AZ_RELEASE_BUILD)
    }

    ProfileScope::ProfileScope(Budget* budget, const char* eventName, ...)
        : m_budget{ budget }
    {
        va_list args;
        va_start(args, eventName);
        BeginRegion(budget, eventName, args);
    }

    ProfileScope::~ProfileScope()
    {
        EndRegion(m_budget);
    }

    template<typename T>
    void Profiler::ReportCounter(
        [[maybe_unused]] const Budget* budget, [[maybe_unused]] const wchar_t* counterName, [[maybe_unused]] const T& value)
    {
#if !defined(AZ_RELEASE_BUILD)
        Platform::ReportCounter(budget, counterName, value);
#endif // !defined(AZ_RELEASE_BUILD)
    }

    // Explicitly instantiate ReportCounter<T>-function for all possible integral types
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const bool& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const float& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const double& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::s8& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::s16& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::s32& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::s64& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::u8& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::u16& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::u32& value);
    template void Profiler::ReportCounter(const Budget* budget, const wchar_t* counterName, const AZ::u64& value);

    void Profiler::ReportProfileEvent([[maybe_unused]] const Budget* budget, [[maybe_unused]] const char* eventName)
    {
#if !defined(AZ_RELEASE_BUILD)
        Platform::ReportProfileEvent(budget, eventName);
#endif // !defined(AZ_RELEASE_BUILD)
    }
} // namespace AZ::Debug
