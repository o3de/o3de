/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
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
        float result = 0.0f;

        if (!m_configuration.m_surfaceTagList.empty())
        {
            SurfaceData::SurfacePointList points;
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePoints,
                params.m_position, m_configuration.m_surfaceTagList, points);

            result = GetMaxSurfaceWeight(points);
        }

        return result;
    }

    void SurfaceMaskGradientComponent::GetValues(AZStd::span<AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        bool valuesFound = false;

        if (!m_configuration.m_surfaceTagList.empty())
        {
            // Rather than calling GetSurfacePoints on the EBus repeatedly in a loop, we instead pass a lambda into the EBus that contains
            // the loop within it so that we can avoid the repeated EBus-calling overhead.
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                [this, positions, &outValues, &valuesFound](SurfaceData::SurfaceDataSystemRequestBus::Events* surfaceDataRequests)
                {
                    // It's possible that there's nothing connected to the EBus, so keep track of the fact that we have valid results.
                    valuesFound = true;
                    SurfaceData::SurfacePointList points;

                    for (size_t index = 0; index < positions.size(); index++)
                    {
                        points.clear();
                        surfaceDataRequests->GetSurfacePoints(positions[index], m_configuration.m_surfaceTagList, points);
                        outValues[index] = GetMaxSurfaceWeight(points);
                    }
                });
        }

        if (!valuesFound)
        {
            // No surface tags, so no output values.
            memset(outValues.data(), 0, outValues.size() * sizeof(float));
        }

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
