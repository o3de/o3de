/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "GradientSignal_precompiled.h"
#include "GradientSignalSystemComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <GradientSignal/ImageAsset.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/SmoothStep.h>
#include <GradientSignal/ImageSettings.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>

namespace GradientSignal
{
    namespace Details
    {
        AzFramework::GenericAssetHandler<GradientSignal::ImageSettings>* s_gradientImageSettingsAssetHandler = nullptr;
        ImageAssetHandler* s_gradientImageAssetHandler = nullptr;
        
        void RegisterAssethandlers()
        {
            s_gradientImageSettingsAssetHandler = aznew AzFramework::GenericAssetHandler<GradientSignal::ImageSettings>("Gradient Image Settings", "Other", s_gradientImageSettingsExtension);
            s_gradientImageSettingsAssetHandler->Register();
            s_gradientImageAssetHandler = aznew ImageAssetHandler();
            s_gradientImageAssetHandler->Register();
        }

        void UnregisterAssethandlers()
        {
            if (s_gradientImageSettingsAssetHandler)
            {
                s_gradientImageSettingsAssetHandler->Unregister();
                delete s_gradientImageSettingsAssetHandler;
                s_gradientImageSettingsAssetHandler = nullptr;
            }

            if (s_gradientImageAssetHandler)
            {
                s_gradientImageAssetHandler->Unregister();
                delete s_gradientImageAssetHandler;
                s_gradientImageAssetHandler = nullptr;
            }
        }
    }

    void GradientSignalSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientSampler::Reflect(context);
        SmoothStep::Reflect(context);
        ImageAsset::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GradientSignalSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<GradientSignalSystemComponent>("GradientSignal", "Manages registration of gradient image assets and reflection of required types")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GradientSampleParams>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("position", BehaviorValueProperty(&GradientSampleParams::m_position))
                ;

            behaviorContext->EBus<GradientRequestBus>("GradientRequestBus")
                ->Event("GetValue", &GradientRequestBus::Events::GetValue)
                ;
        }
    }

    void GradientSignalSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("GradientSignalService", 0xa7a775e2));
    }

    void GradientSignalSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("GradientSignalService", 0xa7a775e2));
    }

    void GradientSignalSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void GradientSignalSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void GradientSignalSystemComponent::Init()
    {
    }

    void GradientSignalSystemComponent::Activate()
    {
        Details::RegisterAssethandlers();
    }

    void GradientSignalSystemComponent::Deactivate()
    {
        Details::UnregisterAssethandlers();
    }
}
