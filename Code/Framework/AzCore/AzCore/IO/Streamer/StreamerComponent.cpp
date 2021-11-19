/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/DedicatedCache.h>
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/Scheduler.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StorageDrive.h>
#include <AzCore/IO/Streamer/ReadSplitter.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/UserSettings/UserSettings.h>

namespace AZ
{
    AZ_CVAR(bool, cl_streamerDevMode, false, nullptr, ConsoleFunctorFlags::Null,
        "Configures AZ::IO::Streamer for use during development. "
        "This includes optimizations to run from loose files and repeatedly reading the same files.");
    AZ_CVAR(AZ::CVarFixedString, cl_streamerProfile, "", nullptr, ConsoleFunctorFlags::Null,
        "Overrides the profile provided by the hardware.");

    StreamerComponent::StreamerComponent()
    {
        // Use platform appropriate defaults for the threading information.
        AZStd::thread_desc threadDesc;
        m_deviceThreadCpuId = threadDesc.m_cpuId;
        m_deviceThreadPriority = threadDesc.m_priority;

#if (AZ_TRAIT_SET_STREAMER_COMPONENT_THREAD_AFFINITY_TO_USERTHREADS)
        m_deviceThreadCpuId = AFFINITY_MASK_USERTHREADS;
#endif
    }

    AZStd::unique_ptr<AZ::IO::Scheduler> StreamerComponent::CreateSimpleStreamerStack()
    {
        return AZStd::make_unique<AZ::IO::Scheduler>(AZStd::make_shared<AZ::IO::StorageDrive>(1024));
    }

    AZStd::unique_ptr<AZ::IO::Scheduler> StreamerComponent::CreateStreamerStack(AZStd::string_view profile)
    {
        AZ::IO::StreamerConfig config;
        auto settingsRegistry = AZ::SettingsRegistry::Get();

        if (!settingsRegistry)
        {
            return CreateSimpleStreamerStack();
        }

        bool useAllHardware = true;
        settingsRegistry->Get(useAllHardware, "/Amazon/AzCore/Streamer/UseAllHardware");
        bool reportHardware = true;
        settingsRegistry->Get(reportHardware, "/Amazon/AzCore/Streamer/ReportHardware");

        AZ::IO::HardwareInformation hardwareInfo;
        if (!AZ::IO::CollectIoHardwareInformation(hardwareInfo, useAllHardware, reportHardware))
        {
            AZ_Assert(false, "Unable to collect information on available IO hardware.");
            return CreateSimpleStreamerStack();
        }

        if (profile.empty())
        {
            profile = hardwareInfo.m_profile;
        }
        AZStd::string profilePath = "/Amazon/AzCore/Streamer/Profiles/";
        profilePath += profile;

        if (!settingsRegistry->GetObject(config, profilePath))
        {
            AZ_Printf("Streamer",
                "Unable to find a definition for profile '%.*s' at /Amazon/AzCore/Streamer/Profiles. Falling back to 'Generic' profile.\n",
                AZ_STRING_ARG(profile));

            if (!settingsRegistry->GetObject(config, "/Amazon/AzCore/Streamer/Profiles/Generic"))
            {
                AZ_Warning("Streamer", false, "Unable to find a definition for profile 'Generic' at /Amazon/AzCore/Streamer/Profiles.");
                return CreateSimpleStreamerStack();
            }
        }

        AZStd::shared_ptr<AZ::IO::StreamStackEntry> stack;
        for (AZStd::shared_ptr<AZ::IO::IStreamerStackConfig>& stackEntryConfig : config.m_stackConfig)
        {
            stack = stackEntryConfig->AddStreamStackEntry(hardwareInfo, AZStd::move(stack));
        }
        if (stack)
        {
            return AZStd::make_unique<AZ::IO::Scheduler>(AZStd::move(stack), hardwareInfo.m_maxPhysicalSectorSize,
                hardwareInfo.m_maxLogicalSectorSize, hardwareInfo.m_maxTransfer);
        }
        else
        {
            AZ_Warning("Streamer", false, "Profile for AZ::IO::Streamer did not contain any valid stack entries.");
            return CreateSimpleStreamerStack();
        }
    }

    void StreamerComponent::Activate()
    {
        AZ_Assert(Interface<IO::IStreamer>::Get() == nullptr, "Streamer has already been activated.");

        AZStd::thread_desc threadDesc;
        if (m_deviceThreadCpuId != threadDesc.m_cpuId)
        {
            threadDesc.m_cpuId = m_deviceThreadCpuId;
        }
        if (m_deviceThreadPriority != threadDesc.m_priority)
        {
            threadDesc.m_priority = m_deviceThreadPriority;
        }

        AZStd::string_view profile;
        AZ::CVarFixedString profileCVar = static_cast<AZ::CVarFixedString>(cl_streamerProfile);
        if (!profileCVar.empty())
        {
            profile = profileCVar;
        }
        else if (cl_streamerDevMode)
        {
            profile = "DevMode";
        }

        m_streamer = AZStd::make_unique<AZ::IO::Streamer>(threadDesc, CreateStreamerStack(profile));
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        TickBus::Handler::BusConnect();
#endif
        Interface<IO::IStreamer>::Register(m_streamer.get());
    }

    void StreamerComponent::Deactivate()
    {
        AZ_Assert(Interface<IO::IStreamer>::Get() != nullptr, "StreamerComponent didn't find an active Streamer during deactivation.");
        Interface<IO::IStreamer>::Unregister(m_streamer.get());
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        TickBus::Handler::BusDisconnect();
#endif
        m_streamer.reset();
    }

    void StreamerComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        bool isEnabled = false;
        if (auto profilerSystem = AZ::Debug::ProfilerSystemInterface::Get(); profilerSystem)
        {
            isEnabled = profilerSystem->IsActive();
        }

        if (isEnabled)
        {
            m_streamer->RecordStatistics();
        }
    }

    void StreamerComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DataStreamingService"));
    }

    void StreamerComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DataStreamingService"));
    }

    void StreamerComponent::GetDependentServices([[maybe_unused]] ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void StreamerComponent::Reflect(ReflectContext* context)
    {
        using namespace AZ::IO;

        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StreamerComponent, Component>()->Version(1);
        }

        BlockCacheConfig::Reflect(context);
        DedicatedCacheConfig::Reflect(context);
        IStreamerStackConfig::Reflect(context);
        FullFileDecompressorConfig::Reflect(context);
        ReadSplitterConfig::Reflect(context);
        StorageDriveConfig::Reflect(context);
        StreamerConfig::Reflect(context);
        ReflectNative(context);
    }

    void StreamerComponent::ReportFileLocks(const AZ::ConsoleCommandContainer&)
    {
        if (m_streamer)
        {
            m_streamer->QueueRequest(m_streamer->Report(AZ::IO::FileRequest::ReportData::ReportType::FileLocks));
        }
    }

    void StreamerComponent::FlushCaches(const AZ::ConsoleCommandContainer&)
    {
        if (m_streamer)
        {
            m_streamer->QueueRequest(m_streamer->FlushCaches());
        }
    }
} // namespace AZ
