/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include "SceneColorViewSystemComponent.h"
#include "SceneColorViewFeatureProcessor.h"
#include "SceneColorViewCapturePass.h"
#include <SceneColorView/Configuration.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace SceneColorView
{
    void SceneColorViewSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        Configuration::Reflect(context);
        SceneColorViewFeatureProcessor::Reflect(context);
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<SceneColorViewSystemComponent, AZ::Component>()->Version(0);
        }
    }
    void SceneColorViewSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    { services.push_back(AZ_CRC_CE("SceneColorViewSystemService")); }
    void SceneColorViewSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    { services.push_back(AZ_CRC_CE("SceneColorViewSystemService")); }
    void SceneColorViewSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    { services.push_back(AZ_CRC_CE("RPISystem")); }
    void SceneColorViewSystemComponent::Activate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SceneColorViewFeatureProcessor>();
        AZ::RPI::PassSystemInterface::Get()->AddPassCreator(AZ::Name("SceneColorViewCapturePass"),
            [](const AZ::RPI::PassDescriptor& descriptor) -> AZ::RPI::Ptr<AZ::RPI::Pass>
            { return SceneColorViewCapturePass::Create(descriptor); });
    }
    void SceneColorViewSystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SceneColorViewFeatureProcessor>();
    }
}
