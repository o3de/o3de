/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereTypeIds.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Render/SkyAtmosphereFeatureProcessor.h>
#include <Render/SkyAtmosphereParentPass.h>
#include <Clients/SkyAtmosphereSystemComponent.h>

namespace SkyAtmosphere
{
    AZ_COMPONENT_IMPL(SkyAtmosphereSystemComponent, "SkyAtmosphereSystemComponent",
        SkyAtmosphereSystemComponentTypeId);

    void SkyAtmosphereSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        SkyAtmosphereFeatureProcessor::Reflect(context);
    }

    void SkyAtmosphereSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SkyAtmosphereSystemService"));
    }

    void SkyAtmosphereSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SkyAtmosphereSystemService"));
    }

    void SkyAtmosphereSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void SkyAtmosphereSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    SkyAtmosphereSystemComponent::SkyAtmosphereSystemComponent()
    {
    }

    SkyAtmosphereSystemComponent::~SkyAtmosphereSystemComponent()
    {
    }

    void SkyAtmosphereSystemComponent::Init()
    {
    }

    void SkyAtmosphereSystemComponent::Activate()
    {
        AZ::ApplicationTypeQuery appType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        if (!appType.IsHeadless())
        {
            AZ::RPI::FeatureProcessorFactory::Get()
                ->RegisterFeatureProcessorWithInterface<SkyAtmosphereFeatureProcessor, SkyAtmosphereFeatureProcessorInterface>();
        }

        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
            []()
            {
                const char* passTemplatesFile = "Passes/SkyAtmospherePassTemplates.azasset";
                AZ::RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(passTemplatesFile);
            });
        passSystem->ConnectEvent(m_loadTemplatesHandler);

        // Add Sky Atmosphere Parent pass
        passSystem->AddPassCreator(AZ::Name("SkyAtmosphereParentPass"), &SkyAtmosphereParentPass::Create);

    }

    void SkyAtmosphereSystemComponent::Deactivate()
    {
        AZ::ApplicationTypeQuery appType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        if (!appType.IsHeadless())
        {
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SkyAtmosphereFeatureProcessor>();
        }
        m_loadTemplatesHandler.Disconnect();
    }

} // namespace SkyAtmosphere
