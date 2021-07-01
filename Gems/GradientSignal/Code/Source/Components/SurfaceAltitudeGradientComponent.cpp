/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "SurfaceAltitudeGradientComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace GradientSignal
{
    void SurfaceAltitudeGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceAltitudeGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ShapeEntityId", &SurfaceAltitudeGradientConfig::m_shapeEntityId)
                ->Field("AltitudeMin", &SurfaceAltitudeGradientConfig::m_altitudeMin)
                ->Field("AltitudeMax", &SurfaceAltitudeGradientConfig::m_altitudeMax)
                ->Field("SurfaceTagsToSample", &SurfaceAltitudeGradientConfig::m_surfaceTagsToSample)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceAltitudeGradientConfig>(
                    "Altitude Gradient", "altitude Gradient")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_shapeEntityId, "Pin To Shape Entity Id", "Shape bounds override min/max altitude if specified.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ShapeService", 0xe86aa5fe))
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_altitudeMin, "Altitude Min", "Minimum acceptable surface altitude.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceAltitudeGradientConfig::IsShapeValid)
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_altitudeMax, "Altitude Max", "Maximum acceptable surface altitude.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceAltitudeGradientConfig::IsShapeValid)
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_surfaceTagsToSample, "Surface Tags to track", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceAltitudeGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("shapeEntityId", BehaviorValueProperty(&SurfaceAltitudeGradientConfig::m_shapeEntityId))
                ->Property("altitudeMin", BehaviorValueProperty(&SurfaceAltitudeGradientConfig::m_altitudeMin))
                ->Property("altitudeMax", BehaviorValueProperty(&SurfaceAltitudeGradientConfig::m_altitudeMax))
                ->Method("GetNumTags", &SurfaceAltitudeGradientConfig::GetNumTags)
                ->Method("GetTag", &SurfaceAltitudeGradientConfig::GetTag)
                ->Method("RemoveTag", &SurfaceAltitudeGradientConfig::RemoveTag)
                ->Method("AddTag", &SurfaceAltitudeGradientConfig::AddTag)
                ;
        }
    }

    bool SurfaceAltitudeGradientConfig::IsShapeValid() const
    {
        return m_shapeEntityId.IsValid();
    }

    size_t SurfaceAltitudeGradientConfig::GetNumTags() const
    {
        return m_surfaceTagsToSample.size();
    }

    AZ::Crc32 SurfaceAltitudeGradientConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_surfaceTagsToSample.size() && tagIndex >= 0)
        {
            return m_surfaceTagsToSample[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceAltitudeGradientConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_surfaceTagsToSample.size() && tagIndex >= 0)
        {
            m_surfaceTagsToSample.erase(m_surfaceTagsToSample.begin() + tagIndex);
        }
    }

    void SurfaceAltitudeGradientConfig::AddTag(AZStd::string tag)
    {
        m_surfaceTagsToSample.push_back(SurfaceData::SurfaceTag(tag));
    }

    void SurfaceAltitudeGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void SurfaceAltitudeGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void SurfaceAltitudeGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void SurfaceAltitudeGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceAltitudeGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceAltitudeGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceAltitudeGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceAltitudeGradientComponentTypeId", BehaviorConstant(SurfaceAltitudeGradientComponentTypeId));

            behaviorContext->Class<SurfaceAltitudeGradientComponent>()->RequestBus("SurfaceAltitudeGradientRequestBus");

            behaviorContext->EBus<SurfaceAltitudeGradientRequestBus>("SurfaceAltitudeGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetShapeEntityId", &SurfaceAltitudeGradientRequestBus::Events::GetShapeEntityId)
                ->Event("SetShapeEntityId", &SurfaceAltitudeGradientRequestBus::Events::SetShapeEntityId)
                ->VirtualProperty("ShapeEntityId", "GetShapeEntityId", "SetShapeEntityId")
                ->Event("GetAltitudeMin", &SurfaceAltitudeGradientRequestBus::Events::GetAltitudeMin)
                ->Event("SetAltitudeMin", &SurfaceAltitudeGradientRequestBus::Events::SetAltitudeMin)
                ->VirtualProperty("AltitudeMin", "GetAltitudeMin", "SetAltitudeMin")
                ->Event("GetAltitudeMax", &SurfaceAltitudeGradientRequestBus::Events::GetAltitudeMax)
                ->Event("SetAltitudeMax", &SurfaceAltitudeGradientRequestBus::Events::SetAltitudeMax)
                ->VirtualProperty("AltitudeMax", "GetAltitudeMax", "SetAltitudeMax")
                ->Event("GetNumTags", &SurfaceAltitudeGradientRequestBus::Events::GetNumTags)
                ->Event("GetTag", &SurfaceAltitudeGradientRequestBus::Events::GetTag)
                ->Event("RemoveTag", &SurfaceAltitudeGradientRequestBus::Events::RemoveTag)
                ->Event("AddTag", &SurfaceAltitudeGradientRequestBus::Events::AddTag)
                ;
        }
    }

    SurfaceAltitudeGradientComponent::SurfaceAltitudeGradientComponent(const SurfaceAltitudeGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceAltitudeGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_shapeEntityId);
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceAltitudeGradientRequestBus::Handler::BusConnect(GetEntityId());
        UpdateFromShape();
        m_dirty = false;
    }

    void SurfaceAltitudeGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        GradientRequestBus::Handler::BusDisconnect();
        SurfaceAltitudeGradientRequestBus::Handler::BusDisconnect();
        m_dirty = false;
    }

    bool SurfaceAltitudeGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceAltitudeGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceAltitudeGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceAltitudeGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float SurfaceAltitudeGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        SurfaceData::SurfacePointList points;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePoints,
            sampleParams.m_position, m_configuration.m_surfaceTagsToSample, points);

        if (points.empty())
        {
            return 0.0f;
        }

        const AZ::Vector3& position = points.front().m_position;
        return GetRatio(m_configuration.m_altitudeMin, m_configuration.m_altitudeMax, position.GetZ());
    }

    void SurfaceAltitudeGradientComponent::OnCompositionChanged()
    {
        m_dirty = true;
    }

    void SurfaceAltitudeGradientComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_dirty)
        {
            const auto altitudeMinOld = m_configuration.m_altitudeMin;
            const auto altitudeMaxOld = m_configuration.m_altitudeMax;

            //updating on tick to query shape bus on main thread
            UpdateFromShape();

            //notify observers if content has changed
            if (altitudeMinOld != m_configuration.m_altitudeMin ||
                altitudeMaxOld != m_configuration.m_altitudeMax)
            {
                LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
            }
            m_dirty = false;
        }
    }

    void SurfaceAltitudeGradientComponent::UpdateFromShape()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_configuration.m_shapeEntityId.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(
                AZ::Vector3(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax, AZ::GetMin(m_configuration.m_altitudeMin, m_configuration.m_altitudeMax)),
                AZ::Vector3(AZ::Constants::FloatMax, AZ::Constants::FloatMax, AZ::GetMax(m_configuration.m_altitudeMin, m_configuration.m_altitudeMax)));

            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            if (bounds.IsValid())
            {
                m_configuration.m_altitudeMin = bounds.GetMin().GetZ();
                m_configuration.m_altitudeMax = bounds.GetMax().GetZ();
            }
        }
    }

    AZ::EntityId SurfaceAltitudeGradientComponent::GetShapeEntityId() const
    {
        return m_configuration.m_shapeEntityId;
    }
    void SurfaceAltitudeGradientComponent::SetShapeEntityId(AZ::EntityId entityId)
    {
        m_configuration.m_shapeEntityId = entityId;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceAltitudeGradientComponent::GetAltitudeMin() const
    {
        return m_configuration.m_altitudeMin;
    }

    void SurfaceAltitudeGradientComponent::SetAltitudeMin(float altitudeMin)
    {
        m_configuration.m_altitudeMin = altitudeMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceAltitudeGradientComponent::GetAltitudeMax() const
    {
        return m_configuration.m_altitudeMax;
    }

    void SurfaceAltitudeGradientComponent::SetAltitudeMax(float altitudeMax)
    {
        m_configuration.m_altitudeMax = altitudeMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t SurfaceAltitudeGradientComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 SurfaceAltitudeGradientComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void SurfaceAltitudeGradientComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceAltitudeGradientComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
