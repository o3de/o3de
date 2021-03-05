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
#include "SurfaceMaskGradientComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Debug/Profiler.h>

namespace GradientSignal
{
    void SurfaceMaskGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("SurfaceTagList", &SurfaceMaskGradientConfig::m_surfaceTagList)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceMaskGradientConfig>(
                    "Surface Mask Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceMaskGradientConfig::m_surfaceTagList, "Surface Tag List", "Identifiers used to compare against underlying surfaces.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceMaskGradientConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetNumTags", &SurfaceMaskGradientConfig::GetNumTags)
                ->Method("GetTag", &SurfaceMaskGradientConfig::GetTag)
                ->Method("RemoveTag", &SurfaceMaskGradientConfig::RemoveTag)
                ->Method("AddTag", &SurfaceMaskGradientConfig::AddTag)
                ;
        }
    }

    size_t SurfaceMaskGradientConfig::GetNumTags() const
    {
        return m_surfaceTagList.size();
    }

    AZ::Crc32 SurfaceMaskGradientConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_surfaceTagList.size())
        {
            return m_surfaceTagList[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceMaskGradientConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_surfaceTagList.size())
        {
            m_surfaceTagList.erase(m_surfaceTagList.begin() + tagIndex);
        }
    }

    void SurfaceMaskGradientConfig::AddTag(AZStd::string tag)
    {
        m_surfaceTagList.push_back(SurfaceData::SurfaceTag(tag));
    }

    void SurfaceMaskGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void SurfaceMaskGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void SurfaceMaskGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceMaskGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceMaskGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceMaskGradientComponentTypeId", BehaviorConstant(SurfaceMaskGradientComponentTypeId));

            behaviorContext->Class<SurfaceMaskGradientComponent>()->RequestBus("SurfaceMaskGradientRequestBus");

            behaviorContext->EBus<SurfaceMaskGradientRequestBus>("SurfaceMaskGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetNumTags", &SurfaceMaskGradientRequestBus::Events::GetNumTags)
                ->Event("GetTag", &SurfaceMaskGradientRequestBus::Events::GetTag)
                ->Event("RemoveTag", &SurfaceMaskGradientRequestBus::Events::RemoveTag)
                ->Event("AddTag", &SurfaceMaskGradientRequestBus::Events::AddTag)
                ;
        }
    }

    SurfaceMaskGradientComponent::SurfaceMaskGradientComponent(const SurfaceMaskGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceMaskGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceMaskGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceMaskGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
        SurfaceMaskGradientRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceMaskGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceMaskGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceMaskGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceMaskGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float SurfaceMaskGradientComponent::GetValue(const GradientSampleParams& params) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        float result = 0.0f;

        if (!m_configuration.m_surfaceTagList.empty())
        {
            SurfaceData::SurfacePointList points;
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePoints,
                params.m_position, m_configuration.m_surfaceTagList, points);

            for (const auto& point : points)
            {
                for (const auto& maskPair : point.m_masks)
                {
                    result = AZ::GetMax(AZ::GetClamp(maskPair.second, 0.0f, 1.0f), result);
                }
            }
        }

        return result;
    }

    size_t SurfaceMaskGradientComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 SurfaceMaskGradientComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void SurfaceMaskGradientComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceMaskGradientComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}