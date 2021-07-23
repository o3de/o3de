/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_STREAMER_COMPONENT_H
#define AZCORE_STREAMER_COMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/Streamer/Scheduler.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    /**
     *
     */
    class StreamerComponent
        : public Component
        , public TickBus::Handler
    {
    public:
        AZ_COMPONENT(StreamerComponent, "{DA47D715-2710-49e2-BC94-EF81C311D89C}")

        StreamerComponent();

        static AZStd::unique_ptr<AZ::IO::Scheduler> CreateStreamerStack(AZStd::string_view profile = {});

    private:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \red ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        static AZStd::unique_ptr<AZ::IO::Scheduler> CreateSimpleStreamerStack();

        void ReportFileLocks(const AZ::ConsoleCommandContainer& someStrings);
        void FlushCaches(const AZ::ConsoleCommandContainer& someStrings);

        AZ_CONSOLEFUNC(StreamerComponent, ReportFileLocks, AZ::ConsoleFunctorFlags::Null,
            "Reports the files currently locked by AZ::IO::Streamer");
        AZ_CONSOLEFUNC(StreamerComponent, FlushCaches, AZ::ConsoleFunctorFlags::Null,
            "Flushes all caches used inside AZ::IO::Streamer");
        
        AZStd::unique_ptr<AZ::IO::Streamer> m_streamer;
        int m_deviceThreadCpuId;
        int m_deviceThreadPriority;
    };
}

#endif // AZCORE_STREAMER_COMPONENT_H
#pragma once
