/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Time/TimeSystemComponent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    AZ_CVAR(float, t_scale, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "A scalar amount to adjust time passage by, 1.0 == realtime, 0.5 == half realtime, 2.0 == doubletime");

    void TimeSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TimeSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void TimeSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TimeService"));
    }

    void TimeSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TimeService"));
    }

    TimeSystemComponent::TimeSystemComponent()
    {
        m_lastInvokedTimeMs = static_cast<TimeMs>(AZStd::GetTimeNowMicroSecond() / 1000);
        AZ::Interface<ITime>::Register(this);
        ITimeRequestBus::Handler::BusConnect();
    }

    TimeSystemComponent::~TimeSystemComponent()
    {
        ITimeRequestBus::Handler::BusDisconnect();
        AZ::Interface<ITime>::Unregister(this);
    }

    void TimeSystemComponent::Activate()
    {
        ;
    }

    void TimeSystemComponent::Deactivate()
    {
        ;
    }

    TimeMs TimeSystemComponent::GetElapsedTimeMs() const
    {
        TimeMs currentTime = static_cast<TimeMs>(AZStd::GetTimeNowMicroSecond() / 1000);
        TimeMs deltaTime = currentTime - m_lastInvokedTimeMs;

        if (t_scale != 1.0f)
        {
            float floatDelta = static_cast<float>(deltaTime) * t_scale;
            deltaTime = static_cast<TimeMs>(static_cast<int64_t>(floatDelta));
        }

        m_accumulatedTimeMs += deltaTime;
        m_lastInvokedTimeMs = currentTime;

        return m_accumulatedTimeMs;
    }
}
