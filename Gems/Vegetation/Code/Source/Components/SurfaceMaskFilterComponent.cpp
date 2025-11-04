/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "SurfaceMaskFilterComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Vegetation/InstanceData.h>

#include <Vegetation/Ebuses/DebugNotificationBus.h>

namespace Vegetation
{
    SurfaceMaskFilterConfig::SurfaceMaskFilterConfig()
    {
        // can't add mask defaults here due to an issue with serialization of vectors just appending data without clearing any existing data first.
    }

    void SurfaceMaskFilterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskFilterConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("FilterStage", &SurfaceMaskFilterConfig::m_filterStage)
                ->Field("AllowOverrides", &SurfaceMaskFilterConfig::m_allowOverrides)
                ->Field("InclusiveSurfaceMasks", &SurfaceMaskFilterConfig::m_inclusiveSurfaceMasks)
                ->Field("InclusiveWeightMin", &SurfaceMaskFilterConfig::m_inclusiveWeightMin)
                ->Field("InclusiveWeightMax", &SurfaceMaskFilterConfig::m_inclusiveWeightMax)
                ->Field("ExclusiveSurfaceMasks", &SurfaceMaskFilterConfig::m_exclusiveSurfaceMasks)
                ->Field("ExclusiveWeightMin", &SurfaceMaskFilterConfig::m_exclusiveWeightMin)
                ->Field("ExclusiveWeightMax", &SurfaceMaskFilterConfig::m_exclusiveWeightMax)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceMaskFilterConfig>(
                    "Vegetation Surface Mask Filter", "Vegetation surface mask filtering")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SurfaceMaskFilterConfig::m_filterStage, "Filter Stage", "Determines if filter is applied before (PreProcess) or after (PostProcess) modifiers.")
                    ->EnumAttribute(FilterStage::Default, "Default")
                    ->EnumAttribute(FilterStage::PreProcess, "PreProcess")
                    ->EnumAttribute(FilterStage::PostProcess, "PostProcess")
                    ->DataElement(0, &SurfaceMaskFilterConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Inclusion")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceMaskFilterConfig::m_inclusiveSurfaceMasks, "Surface Tags", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceMaskFilterConfig::m_inclusiveWeightMin, "Weight Min", "Minimum value accepted from input gradient that allows the filter to pass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceMaskFilterConfig::m_inclusiveWeightMax, "Weight Max", "Maximum value accepted from input gradient that allows the filter to pass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Exclusion")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceMaskFilterConfig::m_exclusiveSurfaceMasks, "Surface Tags", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceMaskFilterConfig::m_exclusiveWeightMin, "Weight Min", "Minimum value accepted from input gradient that allows the filter to pass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceMaskFilterConfig::m_exclusiveWeightMax, "Weight Max", "Maximum value accepted from input gradient that allows the filter to pass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceMaskFilterConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("filterStage",
                    [](SurfaceMaskFilterConfig* config) { return (AZ::u8&)(config->m_filterStage); },
                    [](SurfaceMaskFilterConfig* config, const AZ::u8& i) { config->m_filterStage = (FilterStage)i; })
                ->Property("allowOverrides", BehaviorValueProperty(&SurfaceMaskFilterConfig::m_allowOverrides))
                ->Method("GetNumInclusiveTags", &SurfaceMaskFilterConfig::GetNumInclusiveTags)
                ->Method("GetInclusiveTag", &SurfaceMaskFilterConfig::GetInclusiveTag)
                ->Method("RemoveInclusiveTag", &SurfaceMaskFilterConfig::RemoveInclusiveTag)
                ->Method("AddInclusiveTag", &SurfaceMaskFilterConfig::AddInclusiveTag)
                ->Method("GetNumExclusiveTags", &SurfaceMaskFilterConfig::GetNumExclusiveTags)
                ->Method("GetExclusiveTag", &SurfaceMaskFilterConfig::GetExclusiveTag)
                ->Method("RemoveExclusiveTag", &SurfaceMaskFilterConfig::RemoveExclusiveTag)
                ->Method("AddExclusiveTag", &SurfaceMaskFilterConfig::AddExclusiveTag)
                ;
        }
    }


    size_t SurfaceMaskFilterConfig::GetNumInclusiveTags() const
    {
        return m_inclusiveSurfaceMasks.size();
    }

    AZ::Crc32 SurfaceMaskFilterConfig::GetInclusiveTag(int tagIndex) const
    {
        if (tagIndex < m_inclusiveSurfaceMasks.size() && tagIndex >= 0)
        {
            return m_inclusiveSurfaceMasks[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceMaskFilterConfig::RemoveInclusiveTag(int tagIndex)
    {
        if (tagIndex < m_inclusiveSurfaceMasks.size() && tagIndex >= 0)
        {
            m_inclusiveSurfaceMasks.erase(m_inclusiveSurfaceMasks.begin() + tagIndex);
        }
    }

    void SurfaceMaskFilterConfig::AddInclusiveTag(AZStd::string tag)
    {
        m_inclusiveSurfaceMasks.push_back(SurfaceData::SurfaceTag(tag));
    }

    size_t SurfaceMaskFilterConfig::GetNumExclusiveTags() const
    {
        return m_exclusiveSurfaceMasks.size();
    }

    AZ::Crc32 SurfaceMaskFilterConfig::GetExclusiveTag(int tagIndex) const
    {
        if (tagIndex < m_exclusiveSurfaceMasks.size() && tagIndex >= 0)
        {
            return m_exclusiveSurfaceMasks[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceMaskFilterConfig::RemoveExclusiveTag(int tagIndex)
    {
        if (tagIndex < m_exclusiveSurfaceMasks.size() && tagIndex >= 0)
        {
            m_exclusiveSurfaceMasks.erase(m_exclusiveSurfaceMasks.begin() + tagIndex);
        }
    }

    void SurfaceMaskFilterConfig::AddExclusiveTag(AZStd::string tag)
    {
        m_exclusiveSurfaceMasks.push_back(SurfaceData::SurfaceTag(tag));
    }

    void SurfaceMaskFilterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationFilterService"));
        services.push_back(AZ_CRC_CE("VegetationSurfaceMaskFilterService"));
    }

    void SurfaceMaskFilterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationSurfaceMaskFilterService"));
    }

    void SurfaceMaskFilterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void SurfaceMaskFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceMaskFilterConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskFilterComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceMaskFilterComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceMaskFilterComponentTypeId", BehaviorConstant(SurfaceMaskFilterComponentTypeId));

            behaviorContext->Class<SurfaceMaskFilterComponent>()->RequestBus("SurfaceMaskFilterRequestBus");

            behaviorContext->EBus<SurfaceMaskFilterRequestBus>("SurfaceMaskFilterRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &SurfaceMaskFilterRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &SurfaceMaskFilterRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetNumInclusiveTags", &SurfaceMaskFilterRequestBus::Events::GetNumInclusiveTags)
                ->Event("GetInclusiveTag", &SurfaceMaskFilterRequestBus::Events::GetInclusiveTag)
                ->Event("RemoveInclusiveTag", &SurfaceMaskFilterRequestBus::Events::RemoveInclusiveTag)
                ->Event("AddInclusiveTag", &SurfaceMaskFilterRequestBus::Events::AddInclusiveTag)
                ->Event("GetNumExclusiveTags", &SurfaceMaskFilterRequestBus::Events::GetNumExclusiveTags)
                ->Event("GetExclusiveTag", &SurfaceMaskFilterRequestBus::Events::GetExclusiveTag)
                ->Event("RemoveExclusiveTag", &SurfaceMaskFilterRequestBus::Events::RemoveExclusiveTag)
                ->Event("AddExclusiveTag", &SurfaceMaskFilterRequestBus::Events::AddExclusiveTag)
                ;
        }
    }

    SurfaceMaskFilterComponent::SurfaceMaskFilterComponent(const SurfaceMaskFilterConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceMaskFilterComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        FilterRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceMaskFilterRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceMaskFilterComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        FilterRequestBus::Handler::BusDisconnect();
        SurfaceMaskFilterRequestBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceMaskFilterComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceMaskFilterConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceMaskFilterComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceMaskFilterConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void SurfaceMaskFilterComponent::GetInclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags, bool& includeAll) const
    {
        tags.insert(tags.end(), m_configuration.m_inclusiveSurfaceMasks.begin(), m_configuration.m_inclusiveSurfaceMasks.end());

        // If the include list has no valid tags, that means "include everything"
        if (!SurfaceData::HasValidTags(m_configuration.m_inclusiveSurfaceMasks))
        {
            includeAll = true;
        }
    }

    void SurfaceMaskFilterComponent::GetExclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags) const
    {
        tags.insert(tags.end(), m_configuration.m_exclusiveSurfaceMasks.begin(), m_configuration.m_exclusiveSurfaceMasks.end());
    }

    bool SurfaceMaskFilterComponent::Evaluate(const InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        //determine if tags provided by the component should be considered
        bool useCompTags = !m_configuration.m_allowOverrides || (instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_surfaceFilterOverrideMode != OverrideMode::Replace);

        //determine if tags provided by the descriptor should be considered
        bool useDescTags = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr->m_surfaceFilterOverrideMode != OverrideMode::Disable;

        // if any tags at the current location are to be excluded, reject this instance. rejection always takes priority over inclusion
        const float exclusiveWeightMin = AZ::GetMin(m_configuration.m_exclusiveWeightMin, m_configuration.m_exclusiveWeightMax);
        const float exclusiveWeightMax = AZ::GetMax(m_configuration.m_exclusiveWeightMin, m_configuration.m_exclusiveWeightMax);

        if (useCompTags &&
            instanceData.m_masks.HasAnyMatchingTags(m_configuration.m_exclusiveSurfaceMasks, exclusiveWeightMin, exclusiveWeightMax))
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("SurfaceMaskFilter")));
            return false;
        }

        if (useDescTags &&
            instanceData.m_masks.HasAnyMatchingTags(
                instanceData.m_descriptorPtr->m_exclusiveSurfaceFilterTags, exclusiveWeightMin, exclusiveWeightMax))
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("SurfaceMaskFilter")));
            return false;
        }

        // if any tags at the current location are to be included, accept this instance.
        const float inclusiveWeightMin = AZ::GetMin(m_configuration.m_inclusiveWeightMin, m_configuration.m_inclusiveWeightMax);
        const float inclusiveWeightMax = AZ::GetMax(m_configuration.m_inclusiveWeightMin, m_configuration.m_inclusiveWeightMax);

        if (useCompTags &&
            instanceData.m_masks.HasAnyMatchingTags(m_configuration.m_inclusiveSurfaceMasks, inclusiveWeightMin, inclusiveWeightMax))
        {
            return true;
        }

        if (useDescTags &&
            instanceData.m_masks.HasAnyMatchingTags(
                instanceData.m_descriptorPtr->m_inclusiveSurfaceFilterTags, inclusiveWeightMin, inclusiveWeightMax))
        {
            return true;
        }

        // at this point, nothing was accepted or rejected, which could mean no tags were specified or found, so reject if the inclusion set wasn't empty
        const bool result = (!useCompTags || !SurfaceData::HasValidTags(m_configuration.m_inclusiveSurfaceMasks)) && (!useDescTags || !SurfaceData::HasValidTags(instanceData.m_descriptorPtr->m_inclusiveSurfaceFilterTags));
        if (!result)
        {
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(&DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("SurfaceMaskFilter")));
        }
        return result;
    }

    FilterStage SurfaceMaskFilterComponent::GetFilterStage() const
    {
        return m_configuration.m_filterStage;
    }

    void SurfaceMaskFilterComponent::SetFilterStage(FilterStage filterStage)
    {
        m_configuration.m_filterStage = filterStage;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool SurfaceMaskFilterComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void SurfaceMaskFilterComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t SurfaceMaskFilterComponent::GetNumInclusiveTags() const
    {
        return m_configuration.GetNumInclusiveTags();
    }

    AZ::Crc32 SurfaceMaskFilterComponent::GetInclusiveTag(int tagIndex) const
    {
        return m_configuration.GetInclusiveTag(tagIndex);
    }

    void SurfaceMaskFilterComponent::RemoveInclusiveTag(int tagIndex)
    {
        m_configuration.RemoveInclusiveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceMaskFilterComponent::AddInclusiveTag(AZStd::string tag)
    {
        m_configuration.AddInclusiveTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t SurfaceMaskFilterComponent::GetNumExclusiveTags() const
    {
        return m_configuration.GetNumExclusiveTags();
    }

    AZ::Crc32 SurfaceMaskFilterComponent::GetExclusiveTag(int tagIndex) const
    {
        return m_configuration.GetExclusiveTag(tagIndex);
    }

    void SurfaceMaskFilterComponent::RemoveExclusiveTag(int tagIndex)
    {
        m_configuration.RemoveExclusiveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceMaskFilterComponent::AddExclusiveTag(AZStd::string tag)
    {
        m_configuration.AddExclusiveTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceMaskFilterComponent::GetInclusiveWeightMin() const
    {
        return m_configuration.m_inclusiveWeightMin;
    }

    void SurfaceMaskFilterComponent::SetInclusiveWeightMin(float value)
    {
        m_configuration.m_inclusiveWeightMin = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceMaskFilterComponent::GetInclusiveWeightMax() const
    {
        return m_configuration.m_inclusiveWeightMax;
    }

    void SurfaceMaskFilterComponent::SetInclusiveWeightMax(float value)
    {
        m_configuration.m_inclusiveWeightMax = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceMaskFilterComponent::GetExclusiveWeightMin() const
    {
        return m_configuration.m_exclusiveWeightMin;
    }

    void SurfaceMaskFilterComponent::SetExclusiveWeightMin(float value)
    {

        m_configuration.m_exclusiveWeightMin = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceMaskFilterComponent::GetExclusiveWeightMax() const
    {
        return m_configuration.m_exclusiveWeightMax;
    }

    void SurfaceMaskFilterComponent::SetExclusiveWeightMax(float value)
    {
        m_configuration.m_exclusiveWeightMax = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
