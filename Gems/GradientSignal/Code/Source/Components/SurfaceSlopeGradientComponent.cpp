/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "SurfaceSlopeGradientComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace GradientSignal
{
    void SurfaceSlopeGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceSlopeGradientConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("SurfaceTagsToSample", &SurfaceSlopeGradientConfig::m_surfaceTagsToSample)
                ->Field("SlopeMin", &SurfaceSlopeGradientConfig::m_slopeMin)
                ->Field("SlopeMax", &SurfaceSlopeGradientConfig::m_slopeMax)
                ->Field("RampType", &SurfaceSlopeGradientConfig::m_rampType)
                ->Field("SmoothStep", &SurfaceSlopeGradientConfig::m_smoothStep)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceSlopeGradientConfig>(
                    "Slope Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceSlopeGradientConfig::m_surfaceTagsToSample, "Surface Tags to track", "")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceSlopeGradientConfig::m_slopeMin, "Slope Min", "Minimum surface slope angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SurfaceSlopeGradientConfig::m_slopeMax, "Slope Max", "Maximum surface slope angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SurfaceSlopeGradientConfig::m_rampType, "Ramp Type", "Type of ramp to apply to the slope.")
                    ->EnumAttribute(SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_DOWN, "Linear Ramp Down")
                    ->EnumAttribute(SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_UP, "Linear Ramp Up")
                    ->EnumAttribute(SurfaceSlopeGradientConfig::RampType::SMOOTH_STEP, "Smooth Step")
                    // Note: ReadOnly doesn't currently propagate to children, so instead we hide/show smooth step parameters when 
                    // we change the ramp type.  If ReadOnly is ever changed to propagate downwards, we should change the next line
                    // to PropertyRefreshLevels::AttributesAndLevels and change the Visibility line below on m_smoothStep
                    // to AZ::Edit::PropertyVisibility::Show.
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SurfaceSlopeGradientConfig::m_smoothStep, "Smooth Step Settings", "Parameters for controlling the smooth-step curve.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &SurfaceSlopeGradientConfig::GetSmoothStepParameterVisibility)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceSlopeGradientConfig::IsSmoothStepReadOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceSlopeGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("slopeMin", BehaviorValueProperty(&SurfaceSlopeGradientConfig::m_slopeMin))
                ->Property("slopeMax", BehaviorValueProperty(&SurfaceSlopeGradientConfig::m_slopeMax))
                ->Property("rampType",
                    [](SurfaceSlopeGradientConfig* config) { return reinterpret_cast<AZ::u8&>(config->m_rampType); },
                    [](SurfaceSlopeGradientConfig* config, const AZ::u8& i) { config->m_rampType = static_cast<SurfaceSlopeGradientConfig::RampType>(i); })
                ->Property("smoothStep", BehaviorValueProperty(&SurfaceSlopeGradientConfig::m_smoothStep))
                ->Method("GetNumTags", &SurfaceSlopeGradientConfig::GetNumTags)
                ->Method("GetTag", &SurfaceSlopeGradientConfig::GetTag)
                ->Method("RemoveTag", &SurfaceSlopeGradientConfig::RemoveTag)
                ->Method("AddTag", &SurfaceSlopeGradientConfig::AddTag)
                ;
        }
    }

    size_t SurfaceSlopeGradientConfig::GetNumTags() const
    {
        return m_surfaceTagsToSample.size();
    }

    AZ::Crc32 SurfaceSlopeGradientConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_surfaceTagsToSample.size())
        {
            return m_surfaceTagsToSample[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceSlopeGradientConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_surfaceTagsToSample.size())
        {
            m_surfaceTagsToSample.erase(m_surfaceTagsToSample.begin() + tagIndex);
        }
    }

    void SurfaceSlopeGradientConfig::AddTag(AZStd::string tag)
    {
        m_surfaceTagsToSample.push_back(SurfaceData::SurfaceTag(tag));
    }

    void SurfaceSlopeGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void SurfaceSlopeGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void SurfaceSlopeGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void SurfaceSlopeGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceSlopeGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceSlopeGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceSlopeGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceSlopeGradientComponentTypeId", BehaviorConstant(SurfaceSlopeGradientComponentTypeId));

            behaviorContext->Class<SurfaceSlopeGradientComponent>()->RequestBus("SurfaceSlopeGradientRequestBus");

            behaviorContext->EBus<SurfaceSlopeGradientRequestBus>("SurfaceSlopeGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetSlopeMin", &SurfaceSlopeGradientRequestBus::Events::GetSlopeMin)
                ->Event("SetSlopeMin", &SurfaceSlopeGradientRequestBus::Events::SetSlopeMin)
                ->VirtualProperty("SlopeMin", "GetSlopeMin", "SetSlopeMin")
                ->Event("GetSlopeMax", &SurfaceSlopeGradientRequestBus::Events::GetSlopeMax)
                ->Event("SetSlopeMax", &SurfaceSlopeGradientRequestBus::Events::SetSlopeMax)
                ->VirtualProperty("SlopeMax", "GetSlopeMax", "SetSlopeMax")
                ->Event("GetNumTags", &SurfaceSlopeGradientRequestBus::Events::GetNumTags)
                ->Event("GetTag", &SurfaceSlopeGradientRequestBus::Events::GetTag)
                ->Event("RemoveTag", &SurfaceSlopeGradientRequestBus::Events::RemoveTag)
                ->Event("AddTag", &SurfaceSlopeGradientRequestBus::Events::AddTag)
                ->Event("GetRampType", &SurfaceSlopeGradientRequestBus::Events::GetRampType)
                ->Event("SetRampType", &SurfaceSlopeGradientRequestBus::Events::SetRampType)
                ->VirtualProperty("RampType", "GetRampType", "SetRampType")
                ;
        }
    }

    SurfaceSlopeGradientComponent::SurfaceSlopeGradientComponent(const SurfaceSlopeGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceSlopeGradientComponent::Activate()
    {
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceSlopeGradientRequestBus::Handler::BusConnect(GetEntityId());
        SmoothStepRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceSlopeGradientComponent::Deactivate()
    {
        GradientRequestBus::Handler::BusDisconnect();
        SurfaceSlopeGradientRequestBus::Handler::BusDisconnect();
        SmoothStepRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceSlopeGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceSlopeGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceSlopeGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceSlopeGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float SurfaceSlopeGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        SurfaceData::SurfacePointList points;
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePoints,
            sampleParams.m_position, m_configuration.m_surfaceTagsToSample, points);

        if (points.empty())
        {
            return 0.0f;
        }

        // Assuming our surface normal vector is actually normalized, we can get the slope
        // by just grabbing the Z value.  It's the same thing as normal.Dot(AZ::Vector3::CreateAxisZ()).
        AZ_Assert(points.front().m_normal.GetNormalized().IsClose(points.front().m_normal), "Surface normals are expected to be normalized");
        const float slope = points.front().m_normal.GetZ();
        // Convert slope back to an angle so that we can lerp in "angular space", not "slope value space".
        // (We want our 0-1 range to be linear across the range of angles)
        const float slopeAngle = acosf(slope);

        const float angleMin = AZ::DegToRad(AZ::GetClamp(m_configuration.m_slopeMin, 0.0f, 90.0f));
        const float angleMax = AZ::DegToRad(AZ::GetClamp(m_configuration.m_slopeMax, 0.0f, 90.0f));

        switch (m_configuration.m_rampType)
        {
            case SurfaceSlopeGradientConfig::RampType::SMOOTH_STEP:
                return m_configuration.m_smoothStep.GetSmoothedValue(GetRatio(angleMin, angleMax, slopeAngle));
            case SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_UP:
                // For ramp up, linearly interpolate from min to max.
                return GetRatio(angleMin, angleMax, slopeAngle);
            case SurfaceSlopeGradientConfig::RampType::LINEAR_RAMP_DOWN:
            default:
                // For ramp down, linearly interpolate from max to min.
                return GetRatio(angleMax, angleMin, slopeAngle);
        }
    }

    float SurfaceSlopeGradientComponent::GetSlopeMin() const
    {
        return m_configuration.m_slopeMin;
    }

    void SurfaceSlopeGradientComponent::SetSlopeMin(float slopeMin)
    {
        m_configuration.m_slopeMin = slopeMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceSlopeGradientComponent::GetSlopeMax() const
    {
        return m_configuration.m_slopeMax;
    }

    void SurfaceSlopeGradientComponent::SetSlopeMax(float slopeMax)
    {
        m_configuration.m_slopeMax = slopeMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t SurfaceSlopeGradientComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 SurfaceSlopeGradientComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void SurfaceSlopeGradientComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceSlopeGradientComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u8 SurfaceSlopeGradientComponent::GetRampType() const
    {
        return static_cast<AZ::u8>(m_configuration.m_rampType);
    }

    void SurfaceSlopeGradientComponent::SetRampType(AZ::u8 type)
    {
        m_configuration.m_rampType = static_cast<SurfaceSlopeGradientConfig::RampType>(type);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceSlopeGradientComponent::GetFallOffRange() const
    {
        return m_configuration.m_smoothStep.m_falloffRange;
    }

    void SurfaceSlopeGradientComponent::SetFallOffRange(float range)
    {
        m_configuration.m_smoothStep.m_falloffRange = range;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceSlopeGradientComponent::GetFallOffStrength() const
    {
        return m_configuration.m_smoothStep.m_falloffStrength;
    }

    void SurfaceSlopeGradientComponent::SetFallOffStrength(float strength)
    {
        m_configuration.m_smoothStep.m_falloffStrength = strength;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceSlopeGradientComponent::GetFallOffMidpoint() const
    {
        return m_configuration.m_smoothStep.m_falloffMidpoint;
    }

    void SurfaceSlopeGradientComponent::SetFallOffMidpoint(float midpoint)
    {
        m_configuration.m_smoothStep.m_falloffMidpoint = midpoint;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

}
