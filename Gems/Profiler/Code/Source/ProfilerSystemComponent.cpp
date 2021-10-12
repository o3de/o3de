/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Profiler
{
    static constexpr AZ::Crc32 profilerServiceCrc = AZ_CRC_CE("ProfilerService");

    void ProfilerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ProfilerSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ProfilerSystemComponent>("Profiler", "Provides a custom implementation of the AZ::Debug::Profiler interface for capturing performance data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }

        CpuProfilingStatisticsSerializer::Reflect(context);
    }

    void ProfilerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(profilerServiceCrc);
    }

    void ProfilerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(profilerServiceCrc);
    }

    void ProfilerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ProfilerSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ProfilerSystemComponent::ProfilerSystemComponent()
    {
        if (ProfilerInterface::Get() == nullptr)
        {
            ProfilerInterface::Register(this);
        }
    }

    ProfilerSystemComponent::~ProfilerSystemComponent()
    {
        if (ProfilerInterface::Get() == this)
        {
            ProfilerInterface::Unregister(this);
        }
    }

    void ProfilerSystemComponent::Init()
    {
    }

    void ProfilerSystemComponent::Activate()
    {
        ProfilerRequestBus::Handler::BusConnect();

        m_cpuProfiler.Init();
    }

    void ProfilerSystemComponent::Deactivate()
    {
        m_cpuProfiler.Shutdown();

        ProfilerRequestBus::Handler::BusDisconnect();
    }
} // namespace Profiler
