/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArchiveSystemComponent.h"

#include <Archive/ArchiveTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace Archive
{
    AZ_COMPONENT_IMPL(ArchiveSystemComponent, "ArchiveSystemComponent",
        ArchiveSystemComponentTypeId);

    void ArchiveSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ArchiveSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void ArchiveSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ArchiveService"));
    }

    void ArchiveSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ArchiveService"));
    }

    void ArchiveSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ArchiveSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ArchiveSystemComponent::ArchiveSystemComponent()
    {
        if (ArchiveInterface::Get() == nullptr)
        {
            ArchiveInterface::Register(this);
        }
    }

    ArchiveSystemComponent::~ArchiveSystemComponent()
    {
        if (ArchiveInterface::Get() == this)
        {
            ArchiveInterface::Unregister(this);
        }
    }

    void ArchiveSystemComponent::Init()
    {
    }

    void ArchiveSystemComponent::Activate()
    {
        ArchiveRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void ArchiveSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ArchiveRequestBus::Handler::BusDisconnect();
    }

    void ArchiveSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace Archive
