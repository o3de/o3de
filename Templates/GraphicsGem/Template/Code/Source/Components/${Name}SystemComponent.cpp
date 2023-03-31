/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>

#include <Components/${Name}SystemComponent.h>
#include <Components/${Name}ComponentController.h>

#include <Render/${Name}FeatureProcessor.h>

namespace AZ::Render
{
    void ${Name}SystemComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
        {
            serialize->Class<${Name}SystemComponent, Component>()
                ->Version(0)
                ;
        }

        ${Name}FeatureProcessor::Reflect(context);
    }

    void ${Name}SystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${Name}"));
    }

    void ${Name}SystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${Name}"));
    }

    void ${Name}SystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void ${Name}SystemComponent::Activate()
    {
        RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<${Name}FeatureProcessor>();
    }

    void ${Name}SystemComponent::Deactivate()
    {
        RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<${Name}FeatureProcessor>();
    }
}
