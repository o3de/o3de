/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/RandomGradientComponent.h>
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
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("randomSeed", BehaviorValueProperty(&RandomGradientConfig::m_randomSeed))
                ;
        }
    }

    void RandomGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void RandomGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void RandomGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientTransformService"));
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
        // This will immediately call OnGradientTransformChanged and initialize m_gradientTransform.
        GradientTransformNotificationBus::Handler::BusConnect(GetEntityId());

        RandomGradientRequestBus::Handler::BusConnect(GetEntityId());

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RandomGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        RandomGradientRequestBus::Handler::BusDisconnect();
        GradientTransformNotificationBus::Handler::BusDisconnect();
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

    void RandomGradientComponent::OnGradientTransformChanged(const GradientTransform& newTransform)
    {
        AZStd::unique_lock lock(m_queryMutex);
        m_gradientTransform = newTransform;
    }

    float RandomGradientComponent::GetRandomValue(const AZ::Vector3& position, AZStd::size_t seed) const
    {
        // generating stable pseudo-random noise from a position based hash
        float x = position.GetX();
        float y = position.GetY();
        AZStd::size_t result = 0;

        AZStd::hash_combine<float>(result, x * seed + y);
        AZStd::hash_combine<float>(result, y * seed + x);
        AZStd::hash_combine<float>(result, x * y * seed);

        // always returns [0.0,1.0]
        return static_cast<float>(result % std::numeric_limits<AZ::u8>::max()) / static_cast<float>(std::numeric_limits<AZ::u8>::max());
    }


    float RandomGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        AZ::Vector3 uvw = sampleParams.m_position;
        bool wasPointRejected = false;

        m_gradientTransform.TransformPositionToUVW(sampleParams.m_position, uvw, wasPointRejected);

        if (!wasPointRejected)
        {
            const AZStd::size_t seed = m_configuration.m_randomSeed +
                AZStd::size_t(2); // Add 2 to avoid seeds 0 and 1, which can create strange patterns with this particular algorithm

            return GetRandomValue(uvw, seed);
        }

        return 0.0f;
    }

    void RandomGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        AZ::Vector3 uvw;
        bool wasPointRejected = false;
        const AZStd::size_t seed = m_configuration.m_randomSeed +
            AZStd::size_t(2); // Add 2 to avoid seeds 0 and 1, which can create strange patterns with this particular algorithm

        for (size_t index = 0; index < positions.size(); index++)
        {
            m_gradientTransform.TransformPositionToUVW(positions[index], uvw, wasPointRejected);

            if (!wasPointRejected)
            {
                outValues[index] = GetRandomValue(uvw, seed);
            }
            else
            {
                outValues[index] = 0.0f;
            }
        }
    }


    int RandomGradientComponent::GetRandomSeed() const
    {
        return m_configuration.m_randomSeed;
    }

    void RandomGradientComponent::SetRandomSeed(int seed)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_randomSeed = seed;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
