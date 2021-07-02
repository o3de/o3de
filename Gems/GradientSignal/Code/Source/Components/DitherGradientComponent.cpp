/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "DitherGradientComponent.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Util.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    void DitherGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DitherGradientConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("PatternOffset", &DitherGradientConfig::m_patternOffset)
                ->Field("PatternType", &DitherGradientConfig::m_patternType)
                ->Field("UseSystemPointsPerUnit", &DitherGradientConfig::m_useSystemPointsPerUnit)
                ->Field("PointsPerUnit", &DitherGradientConfig::m_pointsPerUnit)
                ->Field("Gradient", &DitherGradientConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<DitherGradientConfig>(
                    "Dither Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &DitherGradientConfig::m_patternOffset, "Pattern Offset", "Shift pattern lookup indices")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DitherGradientConfig::m_patternType, "Pattern Type", "")
                    ->EnumAttribute(DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4, "4x4")
                    ->EnumAttribute(DitherGradientConfig::BayerPatternType::PATTERN_SIZE_8x8, "8x8")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Sample Settings")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &DitherGradientConfig::m_useSystemPointsPerUnit, "Use System Points Per Unit", "Automatically sets points per unit.  Value is equal to Sector Density / Sector Size")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DitherGradientConfig::m_pointsPerUnit, "Points Per Unit", "Scales input position before sampling")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &DitherGradientConfig::IsPointsPerUnitResdOnly)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &DitherGradientConfig::m_gradientSampler, "Gradient", "Input gradient whose values will be dithered.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DitherGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("useSystemPointsPerUnit", BehaviorValueProperty(&DitherGradientConfig::m_useSystemPointsPerUnit))
                ->Property("pointsPerUnit", BehaviorValueProperty(&DitherGradientConfig::m_pointsPerUnit))
                ->Property("patternOffset", BehaviorValueProperty(&DitherGradientConfig::m_patternOffset))
                ->Property("patternType",
                    [](DitherGradientConfig* config) { return (AZ::u8&)(config->m_patternType); },
                    [](DitherGradientConfig* config, const AZ::u8& i) { config->m_patternType = (DitherGradientConfig::BayerPatternType)i; })
                ->Property("gradientSampler", BehaviorValueProperty(&DitherGradientConfig::m_gradientSampler))
                ;
        }
    }

    bool DitherGradientConfig::IsPointsPerUnitResdOnly() const
    {
        return m_useSystemPointsPerUnit;
    }

    void DitherGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void DitherGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void DitherGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void DitherGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        DitherGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<DitherGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &DitherGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("DitherGradientComponentTypeId", BehaviorConstant(DitherGradientComponentTypeId));

            behaviorContext->Class<DitherGradientComponent>()->RequestBus("DitherGradientRequestBus");

            behaviorContext->EBus<DitherGradientRequestBus>("DitherGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetUseSystemPointsPerUnit", &DitherGradientRequestBus::Events::GetUseSystemPointsPerUnit)
                ->Event("SetUseSystemPointsPerUnit", &DitherGradientRequestBus::Events::SetUseSystemPointsPerUnit)
                ->VirtualProperty("UseSystemPointsPerUnit", "GetUseSystemPointsPerUnit", "SetUseSystemPointsPerUnit")
                ->Event("GetPointsPerUnit", &DitherGradientRequestBus::Events::GetPointsPerUnit)
                ->Event("SetPointsPerUnit", &DitherGradientRequestBus::Events::SetPointsPerUnit)
                ->VirtualProperty("PointsPerUnit", "GetPointsPerUnit", "SetPointsPerUnit")
                ->Event("GetPatternOffset", &DitherGradientRequestBus::Events::GetPatternOffset)
                ->Event("SetPatternOffset", &DitherGradientRequestBus::Events::SetPatternOffset)
                ->VirtualProperty("PatternOffset", "GetPatternOffset", "SetPatternOffset")
                ->Event("GetPatternType", &DitherGradientRequestBus::Events::GetPatternType)
                ->Event("SetPatternType", &DitherGradientRequestBus::Events::SetPatternType)
                ->VirtualProperty("PatternType", "GetPatternType", "SetPatternType")
                ->Event("GetGradientSampler", &DitherGradientRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    DitherGradientComponent::DitherGradientComponent(const DitherGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void DitherGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        DitherGradientRequestBus::Handler::BusConnect(GetEntityId());
        SectorDataNotificationBus::Handler::BusConnect();
    }

    void DitherGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
        DitherGradientRequestBus::Handler::BusDisconnect();
        SectorDataNotificationBus::Handler::BusDisconnect();
    }

    bool DitherGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const DitherGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool DitherGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<DitherGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    int PositionToMatrixIndex(float position, int patternSize)
    {
        int result = static_cast<int>(std::floor(fmod(position, static_cast<float>(patternSize))));

        if (result < 0)
        {
            result += patternSize;
        }

        return result;
    }

    float GetDitherValue4x4(const AZ::Vector3& position)
    {
        const int patternSize = 4;
        const int patternSizeSq = patternSize * patternSize;
        const int indexMatrix[patternSizeSq] = {
            0, 8, 2, 10,
            12, 4, 14, 6,
            3, 11, 1, 9,
            15, 7, 13, 5 };

        const int x = PositionToMatrixIndex(position.GetX(), patternSize);
        const int y = PositionToMatrixIndex(position.GetY(), patternSize);
        
        return indexMatrix[patternSize * y + x] / static_cast<float>(patternSizeSq);
    }

    float GetDitherValue8x8(const AZ::Vector3& position)
    {
        const int patternSize = 8;
        const int patternSizeSq = patternSize * patternSize;
        const int indexMatrix[patternSizeSq] = {
            0, 32, 8, 40, 2, 34, 10, 42,
            48, 16, 56, 24, 50, 18, 58, 26,
            12, 44, 4, 36, 14, 46, 6, 38,
            60, 28, 52, 20, 62, 30, 54, 22,
            3, 35, 11, 43, 1, 33, 9, 41,
            51, 19, 59, 27, 49, 17, 57, 25,
            15, 47, 7, 39, 13, 45, 5, 37,
            63, 31, 55, 23, 61, 29, 53, 21 };

        const int x = PositionToMatrixIndex(position.GetX(), patternSize);
        const int y = PositionToMatrixIndex(position.GetY(), patternSize);

        return indexMatrix[patternSize * y + x] / static_cast<float>(patternSizeSq);
    }

    float DitherGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        const AZ::Vector3& coordinate = sampleParams.m_position;

        float pointsPerUnit = m_configuration.m_pointsPerUnit;
        if (m_configuration.m_useSystemPointsPerUnit)
        {
            SectorDataRequestBus::Broadcast(&SectorDataRequestBus::Events::GetPointsPerMeter, pointsPerUnit);
        }
        pointsPerUnit = AZ::GetMax(pointsPerUnit, 0.0001f);

        auto scaledCoordinate = coordinate * pointsPerUnit;
        auto x = std::floor(scaledCoordinate.GetX()) / pointsPerUnit;
        auto y = std::floor(scaledCoordinate.GetY()) / pointsPerUnit;
        auto z = std::floor(scaledCoordinate.GetZ()) / pointsPerUnit;
        AZ::Vector3 flooredCoordinate(x, y, z);

        GradientSampleParams adjustedSampleParams = sampleParams;
        adjustedSampleParams.m_position = flooredCoordinate;
        float value = m_configuration.m_gradientSampler.GetValue(adjustedSampleParams);

        float d = 0.0f;
        switch (m_configuration.m_patternType)
        {
        default:
        case DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4:
            d = GetDitherValue4x4((scaledCoordinate) + m_configuration.m_patternOffset);
            break;
        case DitherGradientConfig::BayerPatternType::PATTERN_SIZE_8x8:
            d = GetDitherValue8x8((scaledCoordinate) + m_configuration.m_patternOffset);
            break;
        }

        return value > d ? 1.0f : 0.0f;
    }

    bool DitherGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    void DitherGradientComponent::OnSectorDataConfigurationUpdated() const
    {
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool DitherGradientComponent::GetUseSystemPointsPerUnit() const
    {
        return m_configuration.m_useSystemPointsPerUnit;
    }

    void DitherGradientComponent::SetUseSystemPointsPerUnit(bool value)
    {
        m_configuration.m_useSystemPointsPerUnit = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float DitherGradientComponent::GetPointsPerUnit() const
    {
        return m_configuration.m_pointsPerUnit;
    }

    void DitherGradientComponent::SetPointsPerUnit(float points)
    {
        m_configuration.m_pointsPerUnit = points;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 DitherGradientComponent::GetPatternOffset() const
    {
        return m_configuration.m_patternOffset;
    }

    void DitherGradientComponent::SetPatternOffset(AZ::Vector3 offset)
    {
        m_configuration.m_patternOffset = offset;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u8 DitherGradientComponent::GetPatternType() const
    {
        return (AZ::u8)m_configuration.m_patternType;
    }

    void DitherGradientComponent::SetPatternType(AZ::u8 type)
    {
        m_configuration.m_patternType = (DitherGradientConfig::BayerPatternType)type;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSampler& DitherGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
