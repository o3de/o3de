/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DescriptorWeightSelectorComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <VegetationProfiler.h>

namespace Vegetation
{
    void DescriptorWeightSelectorConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorWeightSelectorConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("SortBehavior", &DescriptorWeightSelectorConfig::m_sortBehavior)
                ->Field("Gradient", &DescriptorWeightSelectorConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DescriptorWeightSelectorConfig>(
                    "Vegetation Asset Weight Selector", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DescriptorWeightSelectorConfig::m_sortBehavior, "Sort By Weight", "Defines how descriptors will be sorted before gradient is used for selection")
                    ->EnumAttribute(SortBehavior::Unsorted, "Unsorted")
                    ->EnumAttribute(SortBehavior::Ascending, "Ascending (lowest first)")
                    ->EnumAttribute(SortBehavior::Descending, "Descending (highest first)")
                    ->DataElement(0, &DescriptorWeightSelectorConfig::m_gradientSampler, "Gradient", "Gradient mapped to range between 0 and total combined weight of all descriptors.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DescriptorWeightSelectorConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("noiseType",
                    [](DescriptorWeightSelectorConfig* config) { return (AZ::u8&)(config->m_sortBehavior); },
                    [](DescriptorWeightSelectorConfig* config, const AZ::u8& i) { config->m_sortBehavior = (SortBehavior)i; })
                ->Property("gradientSampler", BehaviorValueProperty(&DescriptorWeightSelectorConfig::m_gradientSampler))
                ;
        }
    }

    void DescriptorWeightSelectorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDescriptorSelectorService"));
    }

    void DescriptorWeightSelectorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationDescriptorSelectorService"));
    }

    void DescriptorWeightSelectorComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void DescriptorWeightSelectorComponent::Reflect(AZ::ReflectContext* context)
    {
        DescriptorWeightSelectorConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DescriptorWeightSelectorComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &DescriptorWeightSelectorComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("DescriptorWeightSelectorComponentTypeId", BehaviorConstant(DescriptorWeightSelectorComponentTypeId));

            behaviorContext->Class<DescriptorWeightSelectorComponent>()->RequestBus("DescriptorWeightSelectorRequestBus");

            behaviorContext->EBus<DescriptorWeightSelectorRequestBus>("DescriptorWeightSelectorRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetSortBehavior", &DescriptorWeightSelectorRequestBus::Events::GetSortBehavior)
                ->Event("SetSortBehavior", &DescriptorWeightSelectorRequestBus::Events::SetSortBehavior)
                ->VirtualProperty("SortBehavior", "GetSortBehavior", "SetSortBehavior")
                ->Event("GetGradientSampler", &DescriptorWeightSelectorRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    DescriptorWeightSelectorComponent::DescriptorWeightSelectorComponent(const DescriptorWeightSelectorConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void DescriptorWeightSelectorComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies({ m_configuration.m_gradientSampler.m_gradientId });
        DescriptorSelectorRequestBus::Handler::BusConnect(GetEntityId());
        DescriptorWeightSelectorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DescriptorWeightSelectorComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        DescriptorSelectorRequestBus::Handler::BusDisconnect();
        DescriptorWeightSelectorRequestBus::Handler::BusDisconnect();
    }

    bool DescriptorWeightSelectorComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const DescriptorWeightSelectorConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool DescriptorWeightSelectorComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<DescriptorWeightSelectorConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void DescriptorWeightSelectorComponent::SelectDescriptors(const DescriptorSelectorParams& params, DescriptorPtrVec& descriptors) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        switch (m_configuration.m_sortBehavior)
        {
        default:
        case SortBehavior::Unsorted:
            //defaulting to no sorting as an optimization since descriptors can be presorted
            break;
        case SortBehavior::Ascending:
            std::sort(descriptors.begin(), descriptors.end(), [](const auto& lhs, const auto& rhs) { return lhs->m_weight < rhs->m_weight; });
            break;
        case SortBehavior::Descending:
            std::sort(descriptors.begin(), descriptors.end(), [](const auto& lhs, const auto& rhs) { return lhs->m_weight > rhs->m_weight; });
            break;
        }

        float totalWeight = 0.0f;
        for (const auto& descriptor : descriptors)
        {
            totalWeight += descriptor->m_weight;
        }

        int count = 0;
        const GradientSignal::GradientSampleParams sampleParams(params.m_position);
        float minimumWeight = m_configuration.m_gradientSampler.GetValue(sampleParams) * totalWeight;
        float currentWeight = 0.0f;
        for (const auto& descriptor : descriptors)
        {
            currentWeight += descriptor->m_weight;
            if (currentWeight < minimumWeight)
            {
                ++count;
            }
        }

        if (count > 0)
        {
            descriptors.erase(descriptors.begin(), descriptors.begin() + count);
        }
    }

    SortBehavior DescriptorWeightSelectorComponent::GetSortBehavior() const
    {
        return m_configuration.m_sortBehavior;
    }

    void DescriptorWeightSelectorComponent::SetSortBehavior(SortBehavior behavior)
    {
        m_configuration.m_sortBehavior = behavior;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSignal::GradientSampler& DescriptorWeightSelectorComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
