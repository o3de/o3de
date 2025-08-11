/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AFRSystemComponent.h"

#include <AFR/AFRTypeIds.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AFR/AFRFeatureProcessor.h>

namespace AFR
{
    AZ_COMPONENT_IMPL(AFRSystemComponent, "AFRSystemComponent",
        AFRSystemComponentTypeId);

    void AFRSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AFRFeatureProcessor::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AFRSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void AFRSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AFRService"));
    }

    void AFRSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AFRService"));
    }

    void AFRSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
        required.push_back(AZ_CRC("BootstrapSystemComponent", 0xb8f32711));
    }

    void AFRSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    AFRSystemComponent::AFRSystemComponent()
    {
        if (AFRInterface::Get() == nullptr)
        {
            AFRInterface::Register(this);
        }
    }

    AFRSystemComponent::~AFRSystemComponent()
    {
        if (AFRInterface::Get() == this)
        {
            AFRInterface::Unregister(this);
        }
    }

    void AFRSystemComponent::Init()
    {
    }

    void AFRSystemComponent::Activate()
    {
        AFRRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<AFRFeatureProcessor>();
    }

    void AFRSystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<AFRFeatureProcessor>();

        AZ::TickBus::Handler::BusDisconnect();
        AFRRequestBus::Handler::BusDisconnect();
    }

    void AFRSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace AFR
