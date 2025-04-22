/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/DitherGradientComponent.h>
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
                    ->EndGroup()

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
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void DitherGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
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
        DitherGradientRequestBus::Handler::BusConnect(GetEntityId());
        SectorDataNotificationBus::Handler::BusConnect();

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DitherGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
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

    int DitherGradientComponent::ScaledPositionToPatternIndex(const AZ::Vector3& scaledPosition, int patternSize)
    {
        // The input position is expected to be scaled up so that each integer value is a unique point in our dither pattern, and
        // the fractional value is just the amount within the point.  The output is the specific index into an NxN pattern to use
        // for the dither comparison value.

        // Get the floor before casting to int because we want fractional negative values to go "down" to the next negative value.
        AZ::Vector3 flooredScaledPosition = scaledPosition.GetFloor();

        // For a pattern of 4, we want our indices to go 0, 1, 2, 3, 0, 1, 2, 3, etc. However, we want it continuous across
        // negative and positive positions so we can't just use mod with abs(). Instead, we use a double-mod which gives us
        // a result that's continuous across all coordinate space.
        const int x = ((static_cast<int>(flooredScaledPosition.GetX()) % patternSize) + patternSize) % patternSize;
        const int y = ((static_cast<int>(flooredScaledPosition.GetY()) % patternSize) + patternSize) % patternSize;

        return (patternSize * y + x);
    }

    float DitherGradientComponent::GetDitherValue4x4(const AZ::Vector3& scaledPosition)
    {
        constexpr int patternSize = 4;
        constexpr float indexMatrix[] = {
             0.0f / 16.0f,  8.0f / 16.0f,  2.0f / 16.0f, 10.0f / 16.0f,
            12.0f / 16.0f,  4.0f / 16.0f, 14.0f / 16.0f,  6.0f / 16.0f,
             3.0f / 16.0f, 11.0f / 16.0f,  1.0f / 16.0f,  9.0f / 16.0f,
            15.0f / 16.0f,  7.0f / 16.0f, 13.0f / 16.0f,  5.0f / 16.0f };

        return indexMatrix[ScaledPositionToPatternIndex(scaledPosition, patternSize)];
    }

    float DitherGradientComponent::GetDitherValue8x8(const AZ::Vector3& scaledPosition)
    {
        constexpr int patternSize = 8;
        constexpr float indexMatrix[] = {
             0.0f / 64.0f, 32.0f / 64.0f,  8.0f / 64.0f, 40.0f / 64.0f,  2.0f / 64.0f, 34.0f / 64.0f, 10.0f / 64.0f, 42.0f / 64.0f,
            48.0f / 64.0f, 16.0f / 64.0f, 56.0f / 64.0f, 24.0f / 64.0f, 50.0f / 64.0f, 18.0f / 64.0f, 58.0f / 64.0f, 26.0f / 64.0f,
            12.0f / 64.0f, 44.0f / 64.0f,  4.0f / 64.0f, 36.0f / 64.0f, 14.0f / 64.0f, 46.0f / 64.0f,  6.0f / 64.0f, 38.0f / 64.0f,
            60.0f / 64.0f, 28.0f / 64.0f, 52.0f / 64.0f, 20.0f / 64.0f, 62.0f / 64.0f, 30.0f / 64.0f, 54.0f / 64.0f, 22.0f / 64.0f,
             3.0f / 64.0f, 35.0f / 64.0f, 11.0f / 64.0f, 43.0f / 64.0f,  1.0f / 64.0f, 33.0f / 64.0f,  9.0f / 64.0f, 41.0f / 64.0f,
            51.0f / 64.0f, 19.0f / 64.0f, 59.0f / 64.0f, 27.0f / 64.0f, 49.0f / 64.0f, 17.0f / 64.0f, 57.0f / 64.0f, 25.0f / 64.0f,
            15.0f / 64.0f, 47.0f / 64.0f,  7.0f / 64.0f, 39.0f / 64.0f, 13.0f / 64.0f, 45.0f / 64.0f,  5.0f / 64.0f, 37.0f / 64.0f,
            63.0f / 64.0f, 31.0f / 64.0f, 55.0f / 64.0f, 23.0f / 64.0f, 61.0f / 64.0f, 29.0f / 64.0f, 53.0f / 64.0f, 21.0f / 64.0f
        };

        return indexMatrix[ScaledPositionToPatternIndex(scaledPosition, patternSize)];
    }

    float DitherGradientComponent::GetCalculatedPointsPerUnit() const
    {
        float pointsPerUnit = m_configuration.m_pointsPerUnit;
        if (m_configuration.m_useSystemPointsPerUnit)
        {
            SectorDataRequestBus::Broadcast(&SectorDataRequestBus::Events::GetPointsPerMeter, pointsPerUnit);
        }
        return AZ::GetMax(pointsPerUnit, 0.0001f);
    }

    float DitherGradientComponent::GetDitherValue(const AZ::Vector3& scaledPosition, float value) const
    {
        float d = 0.0f;
        switch (m_configuration.m_patternType)
        {
        default:
        case DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4:
            d = GetDitherValue4x4(scaledPosition + m_configuration.m_patternOffset);
            break;
        case DitherGradientConfig::BayerPatternType::PATTERN_SIZE_8x8:
            d = GetDitherValue8x8(scaledPosition + m_configuration.m_patternOffset);
            break;
        }

        return (value > d) ? 1.0f : 0.0f;
    }

    float DitherGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        const AZ::Vector3& coordinate = sampleParams.m_position;

        const float pointsPerUnit = GetCalculatedPointsPerUnit();

        AZ::Vector3 scaledCoordinate = coordinate * pointsPerUnit;
        AZ::Vector3 flooredCoordinate = scaledCoordinate.GetFloor() / pointsPerUnit;

        GradientSampleParams adjustedSampleParams = sampleParams;
        adjustedSampleParams.m_position = flooredCoordinate;
        float value = m_configuration.m_gradientSampler.GetValue(adjustedSampleParams);

        return GetDitherValue(scaledCoordinate, value);
    }

    void DitherGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        const float pointsPerUnit = GetCalculatedPointsPerUnit();

        // Create the entire set of floored coordinates to use in the gradient value lookups.
        AZStd::vector<AZ::Vector3> flooredCoordinates(positions.size());
        for (size_t index = 0; index < positions.size(); index++)
        {
            AZ::Vector3 scaledCoordinate = positions[index] * pointsPerUnit;
            flooredCoordinates[index] = scaledCoordinate.GetFloor() / pointsPerUnit;
        }

        m_configuration.m_gradientSampler.GetValues(flooredCoordinates, outValues);

        // For each gradient value, turn it into a 0 or 1 based on the location and the dither pattern.
        for (size_t index = 0; index < positions.size(); index++)
        {
            AZ::Vector3 scaledCoordinate = positions[index] * pointsPerUnit;
            outValues[index] = GetDitherValue(scaledCoordinate, outValues[index]);
        }
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
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_useSystemPointsPerUnit = value;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float DitherGradientComponent::GetPointsPerUnit() const
    {
        return m_configuration.m_pointsPerUnit;
    }

    void DitherGradientComponent::SetPointsPerUnit(float points)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_pointsPerUnit = points;
        }
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 DitherGradientComponent::GetPatternOffset() const
    {
        return m_configuration.m_patternOffset;
    }

    void DitherGradientComponent::SetPatternOffset(AZ::Vector3 offset)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_patternOffset = offset;
        }
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::u8 DitherGradientComponent::GetPatternType() const
    {
        return (AZ::u8)m_configuration.m_patternType;
    }

    void DitherGradientComponent::SetPatternType(AZ::u8 type)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_patternType = (DitherGradientConfig::BayerPatternType)type;
        }
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSampler& DitherGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
