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
#include "RandomGradientComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Debug/Profiler.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

namespace GradientSignal
{
    void RandomGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RandomGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("RandomSeed", &RandomGradientConfig::m_randomSeed)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<RandomGradientConfig>(
                    "Random Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RandomGradientConfig::m_randomSeed, "Random Seed", "Seed value for the Random Noise Generator.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<int>::max())
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 100)
                        ->Attribute(AZ::Edit::Attributes::Step, 10)
                    ;
            }
        }


        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RandomGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("randomSeed", BehaviorValueProperty(&RandomGradientConfig::m_randomSeed))
                ;
        }
    }

    void RandomGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void RandomGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void RandomGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void RandomGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        RandomGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RandomGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &RandomGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("RandomGradientComponentTypeId", BehaviorConstant(RandomGradientComponentTypeId));

            behaviorContext->Class<RandomGradientComponent>()->RequestBus("RandomGradientRequestBus");

            behaviorContext->EBus<RandomGradientRequestBus>("RandomGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetRandomSeed", &RandomGradientRequestBus::Events::GetRandomSeed)
                ->Event("SetRandomSeed", &RandomGradientRequestBus::Events::SetRandomSeed)
                ->VirtualProperty("RandomSeed", "GetRandomSeed", "SetRandomSeed")
                ;
        }
    }

    RandomGradientComponent::RandomGradientComponent(const RandomGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void RandomGradientComponent::Activate()
    {
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        RandomGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RandomGradientComponent::Deactivate()
    {
        GradientRequestBus::Handler::BusDisconnect();
        RandomGradientRequestBus::Handler::BusDisconnect();
    }

    bool RandomGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RandomGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool RandomGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<RandomGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float RandomGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZ::Vector3 uvw = sampleParams.m_position;

        bool wasPointRejected = false;
        const bool shouldNormalizeOutput = false;
        GradientTransformRequestBus::Event(
            GetEntityId(), &GradientTransformRequestBus::Events::TransformPositionToUVW, sampleParams.m_position, uvw, shouldNormalizeOutput, wasPointRejected);

        if (!wasPointRejected)
        {
            //generating stable pseudo-random noise from a position based hash 
            float x = uvw.GetX();
            float y = uvw.GetY();
            AZStd::size_t result = 0;
            const AZStd::size_t seed = m_configuration.m_randomSeed + AZStd::size_t(2); // Add 2 to avoid seeds 0 and 1, which can create strange patterns with this particular algorithm

            AZStd::hash_combine<float>(result, x * seed + y);
            AZStd::hash_combine<float>(result, y * seed + x);
            AZStd::hash_combine<float>(result, x * y * seed);

            //always returns [0.0,1.0]
            return static_cast<float>(result % std::numeric_limits<AZ::u8>::max()) / static_cast<float>(std::numeric_limits<AZ::u8>::max());
        }

        return 0.0f;
    }

    int RandomGradientComponent::GetRandomSeed() const
    {
        return m_configuration.m_randomSeed;
    }

    void RandomGradientComponent::SetRandomSeed(int seed)
    {
        m_configuration.m_randomSeed = seed;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}