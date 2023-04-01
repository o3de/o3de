/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}SystemComponent.h"

#include <${Name}/${Name}TypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>

#include <Render/${Name}FeatureProcessor.h>

namespace ${Name}
{
    AZ_COMPONENT_IMPL(${Name}SystemComponent, "${Name}SystemComponent",
        ${Name}SystemComponentTypeId);

    void ${Name}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${Name}SystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        ${Name}FeatureProcessor::Reflect(context);
    }

    void ${Name}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${Name}SystemService"));
    }

    void ${Name}SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${Name}SystemService"));
    }

    void ${Name}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void ${Name}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ${Name}SystemComponent::${Name}SystemComponent()
    {
        if (${Name}Interface::Get() == nullptr)
        {
            ${Name}Interface::Register(this);
        }
    }

    ${Name}SystemComponent::~${Name}SystemComponent()
    {
        if (${Name}Interface::Get() == this)
        {
            ${Name}Interface::Unregister(this);
        }
    }

    void ${Name}SystemComponent::Init()
    {
    }

    void ${Name}SystemComponent::Activate()
    {
        ${Name}RequestBus::Handler::BusConnect();

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<${Name}FeatureProcessor>();
    }

    void ${Name}SystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<${Name}FeatureProcessor>();

        ${Name}RequestBus::Handler::BusDisconnect();
    }

} // namespace ${Name}
