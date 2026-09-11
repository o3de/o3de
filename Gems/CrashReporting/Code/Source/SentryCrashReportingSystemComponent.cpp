/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/SentryCrashReportingSystemComponent.h>

#include <SentryCrashCore/SentryCrashCore.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace CrashReporting
{
    namespace
    {
        AZStd::string ResolveAlias(const char* alias)
        {
            if (AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance())
            {
                if (auto resolved = fileIO->ResolvePath(AZ::IO::PathView(alias)); resolved.has_value())
                {
                    return resolved->c_str();
                }
            }
            return {};
        }

        //! Tags the report with whichever launcher this is, so a client crash is distinguishable
        //! from a server one in Sentry.
        AZStd::string ExecutableModuleTag()
        {
            char buffer[AZ::IO::MaxPathLength]{};
            const auto result = AZ::Utils::GetExecutablePath(buffer, sizeof(buffer));
            // GetExecutablePath is documented as sometimes returning only the directory, in which
            // case Stem() would silently hand back a folder name instead of the executable's.
            if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success
                || !result.m_pathIncludesFilename)
            {
                return "GameLauncher";
            }
            AZStd::string stem{ AZ::IO::PathView(buffer).Stem().Native() };
            return stem.empty() ? AZStd::string("GameLauncher") : stem;
        }
    }

    void SentryCrashReportingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SentryCrashReportingSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void SentryCrashReportingSystemComponent::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CrashReportingService"));
    }

    void SentryCrashReportingSystemComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CrashReportingService"));
    }

    void SentryCrashReportingSystemComponent::Activate()
    {
        const AZStd::string moduleTag = ExecutableModuleTag();

        // The crash database must live somewhere writable; @user@ is the launcher's own user
        // folder, unlike the possibly read-only install directory.
        AZStd::string appRoot = ResolveAlias("@user@");
        if (appRoot.empty())
        {
            appRoot = ResolveAlias("@products@");
        }

        SentryCrash::Config config = SentryCrash::LoadConfig(moduleTag, appRoot);

        // A packaged build does not ship the Qt companion reporter, so crashes upload silently
        // rather than trying to launch a UI that is not there. Leaving this empty disables it.
        config.m_externalReporterPath.clear();
        config.m_attemptLevelSaveOnCrash = false;

        const AZStd::string logDir = ResolveAlias("@log@");
        if (!logDir.empty())
        {
            config.m_attachments.push_back(
                AZStd::string::format("%s/%s.log", logDir.c_str(), moduleTag.c_str()));
        }

        if (!SentryCrash::Initialize(config))
        {
            return;
        }

        if (auto* registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            AZ::IO::FixedMaxPath projectPath;
            if (registry->Get(
                    projectPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
            {
                SentryCrash::SetTag("project_path", projectPath.c_str());
            }
        }

        if (config.m_enableAppHangTracking)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SentryCrashReportingSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        // Flushes queued envelopes and marks the session as a clean exit.
        SentryCrash::Shutdown();
    }

    void SentryCrashReportingSystemComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        SentryCrash::AppHangHeartbeat();
    }

    int SentryCrashReportingSystemComponent::GetTickOrder()
    {
        return AZ::TICK_FIRST;
    }
}
