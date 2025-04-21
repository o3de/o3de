/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <VegetationProfiler.h>
#include "SurfaceMaskDepthFilterComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/InstanceData.h>
#include <Vegetation/Descriptor.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/sort.h>

#include <Vegetation/Ebuses/DebugNotificationBus.h>

namespace Vegetation
{
    void SurfaceMaskDepthFilterConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskDepthFilterConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("FilterStage", &SurfaceMaskDepthFilterConfig::m_filterStage)
                ->Field("AllowOverrides", &SurfaceMaskDepthFilterConfig::m_allowOverrides)
                ->Field("UpperDistanceRange", &SurfaceMaskDepthFilterConfig::m_upperDistance)
                ->Field("LowerDistanceRange", &SurfaceMaskDepthFilterConfig::m_lowerDistance)
                ->Field("DepthComparisonTags", &SurfaceMaskDepthFilterConfig::m_depthComparisonTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceMaskDepthFilterConfig>(
                    "Vegetation Surface Depth Filter", "Filters vegetation based on the depth between two surface mask tags")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SurfaceMaskDepthFilterConfig::m_filterStage, "Filter Stage", "Determines if filter is applied before (PreProcess) or after (PostProcess) modifiers.")
                    ->EnumAttribute(FilterStage::Default, "Default")
                    ->EnumAttribute(FilterStage::PreProcess, "PreProcess")
                    ->EnumAttribute(FilterStage::PostProcess, "PostProcess")
                    ->DataElement(0, &SurfaceMaskDepthFilterConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")
                    ->DataElement(0, &SurfaceMaskDepthFilterConfig::m_upperDistance, "Upper Distance Range", "Highest distance between the comparison tag elevation and the current instance, negative for below")
                    ->DataElement(0, &SurfaceMaskDepthFilterConfig::m_lowerDistance, "Lower Distance Range", "Lowest distance between the comparison tag elevation and the current instance, negative for below")
                    ->DataElement(0, &SurfaceMaskDepthFilterConfig::m_depthComparisonTags, "Depth Comparison Tags", "The surface tag mask to query the elevation to compare against")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceMaskDepthFilterConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("filterStage",
                    [](SurfaceMaskDepthFilterConfig* config) { return (AZ::u8&)(config->m_filterStage); },
                    [](SurfaceMaskDepthFilterConfig* config, const AZ::u8& i) { config->m_filterStage = (FilterStage)i; })
                ->Property("allowOverrides", BehaviorValueProperty(&SurfaceMaskDepthFilterConfig::m_allowOverrides))
                ->Property("lowerDistance", BehaviorValueProperty(&SurfaceMaskDepthFilterConfig::m_lowerDistance))
                ->Property("m_upperDistance", BehaviorValueProperty(&SurfaceMaskDepthFilterConfig::m_upperDistance))
                ->Method("GetNumTags", &SurfaceMaskDepthFilterConfig::GetNumTags)
                ->Method("GetTag", &SurfaceMaskDepthFilterConfig::GetTag)
                ->Method("AddTag", &SurfaceMaskDepthFilterConfig::AddTag)
                ->Method("RemoveTag", &SurfaceMaskDepthFilterConfig::RemoveTag)
                ;
        }
    }

    size_t SurfaceMaskDepthFilterConfig::GetNumTags() const
    {
        return m_depthComparisonTags.size();
    }

    AZ::Crc32 SurfaceMaskDepthFilterConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_depthComparisonTags.size() && tagIndex >= 0)
        {
            return m_depthComparisonTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceMaskDepthFilterConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_depthComparisonTags.size() && tagIndex >= 0)
        {
            m_depthComparisonTags.erase(m_depthComparisonTags.begin() + tagIndex);
        }
    }

    void SurfaceMaskDepthFilterConfig::AddTag(AZStd::string tag)
    {
        m_depthComparisonTags.push_back(SurfaceData::SurfaceTag(tag));
    }


    void SurfaceMaskDepthFilterComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationFilterService"));
        services.push_back(AZ_CRC_CE("VegetationSurfaceMaskDepthFilterService"));
    }

    void SurfaceMaskDepthFilterComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationSurfaceMaskDepthFilterService"));
    }

    void SurfaceMaskDepthFilterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void SurfaceMaskDepthFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceMaskDepthFilterConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskDepthFilterComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceMaskDepthFilterComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceMaskDepthFilterComponentTypeId", BehaviorConstant(SurfaceMaskDepthFilterComponentTypeId));

            behaviorContext->Class<SurfaceMaskDepthFilterComponent>()->RequestBus("SurfaceMaskDepthFilterRequestBus");

            behaviorContext->EBus<SurfaceMaskDepthFilterRequestBus>("SurfaceMaskDepthFilterRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Event("GetAllowOverrides", &SurfaceMaskDepthFilterRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &SurfaceMaskDepthFilterRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetLowerDistance", &SurfaceMaskDepthFilterRequestBus::Events::GetLowerDistance)
                ->Event("SetLowerDistance", &SurfaceMaskDepthFilterRequestBus::Events::SetLowerDistance)
                ->VirtualProperty("LowerDistance", "GetLowerDistance", "SetLowerDistance")
                ->Event("GetUpperDistance", &SurfaceMaskDepthFilterRequestBus::Events::GetUpperDistance)
                ->Event("SetUpperDistance", &SurfaceMaskDepthFilterRequestBus::Events::SetUpperDistance)
                ->VirtualProperty("UpperDistance", "GetUpperDistance", "SetUpperDistance")
                ->Event("GetNumTags", &SurfaceMaskDepthFilterRequestBus::Events::GetNumTags)
                ->Event("GetTag", &SurfaceMaskDepthFilterRequestBus::Events::GetTag)
                ->Event("RemoveTag", &SurfaceMaskDepthFilterRequestBus::Events::RemoveTag)
                ->Event("AddTag", &SurfaceMaskDepthFilterRequestBus::Events::AddTag)
                ;
        }
    }

    SurfaceMaskDepthFilterComponent::SurfaceMaskDepthFilterComponent(const SurfaceMaskDepthFilterConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceMaskDepthFilterComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        FilterRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceMaskDepthFilterRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceMaskDepthFilterComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        FilterRequestBus::Handler::BusDisconnect();
        SurfaceMaskDepthFilterRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceMaskDepthFilterComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceMaskDepthFilterConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceMaskDepthFilterComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceMaskDepthFilterConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SurfaceMaskDepthFilterComponent::Evaluate(const InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && !instanceData.m_descriptorPtr->m_surfaceTagDistance.m_tags.empty();
        const SurfaceData::SurfaceTagVector& surfaceTagsToCompare = useOverrides ? instanceData.m_descriptorPtr->m_surfaceTagDistance.m_tags : m_configuration.m_depthComparisonTags;
        float lowerZDistanceRange = useOverrides ? instanceData.m_descriptorPtr->m_surfaceTagDistance.m_lowerDistanceInMeters : m_configuration.m_lowerDistance;
        float upperZDistanceRange = useOverrides ? instanceData.m_descriptorPtr->m_surfaceTagDistance.m_upperDistanceInMeters : m_configuration.m_upperDistance;

        bool passesFilter = false;

        if (!surfaceTagsToCompare.empty())
        {
            m_points.Clear();
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePoints(instanceData.m_position, surfaceTagsToCompare, m_points);

            float instanceZ = instanceData.m_position.GetZ();
            m_points.EnumeratePoints(
                [instanceZ, lowerZDistanceRange, upperZDistanceRange, &passesFilter](
                    [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position,
                    [[maybe_unused]] const AZ::Vector3& normal, [[maybe_unused]] const SurfaceData::SurfaceTagWeights& masks) -> bool
                {
                    float pointZ = position.GetZ();
                    float zDistance = instanceZ - pointZ;
                    if (lowerZDistanceRange <= zDistance && zDistance <= upperZDistanceRange)
                    {
                        passesFilter = true;
                        return false;
                    }
                    return true;
                });
        }

        if (!passesFilter)
        {
            // if we get here instance is marked filtered
            VEG_PROFILE_METHOD(DebugNotificationBus::TryQueueBroadcast(
                &DebugNotificationBus::Events::FilterInstance, instanceData.m_id, AZStd::string_view("SurfaceDepthMaskFilter")));
        }

        return passesFilter;
    }

    FilterStage SurfaceMaskDepthFilterComponent::GetFilterStage() const
    {
        return m_configuration.m_filterStage;
    }

    void SurfaceMaskDepthFilterComponent::SetFilterStage(FilterStage filterStage)
    {
        m_configuration.m_filterStage = filterStage;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool SurfaceMaskDepthFilterComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void SurfaceMaskDepthFilterComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceMaskDepthFilterComponent::GetLowerDistance() const
    {
        return m_configuration.m_lowerDistance;
    }

    void SurfaceMaskDepthFilterComponent::SetLowerDistance(float lowerDistance)
    {
        m_configuration.m_lowerDistance = lowerDistance;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceMaskDepthFilterComponent::GetUpperDistance() const
    {
        return m_configuration.m_upperDistance;
    }

    void SurfaceMaskDepthFilterComponent::SetUpperDistance(float upperDistance)
    {
        m_configuration.m_upperDistance = upperDistance;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t SurfaceMaskDepthFilterComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 SurfaceMaskDepthFilterComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void SurfaceMaskDepthFilterComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceMaskDepthFilterComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
