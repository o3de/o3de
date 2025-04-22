/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/ShapeAreaFalloffGradientComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Debug/Profiler.h>

namespace GradientSignal
{
    void ShapeAreaFalloffGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ShapeAreaFalloffGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ShapeEntityId", &ShapeAreaFalloffGradientConfig::m_shapeEntityId)
                ->Field("FalloffWidth", &ShapeAreaFalloffGradientConfig::m_falloffWidth)
                ->Field("FalloffType", &ShapeAreaFalloffGradientConfig::m_falloffType)
                ->Field("Is3dFalloff", &ShapeAreaFalloffGradientConfig::m_is3dFalloff)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ShapeAreaFalloffGradientConfig>(
                    "Shape Falloff Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &ShapeAreaFalloffGradientConfig::m_shapeEntityId, "Shape Entity Id", "Entity with shape component to test distance against.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ShapeService"))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ShapeAreaFalloffGradientConfig::m_falloffWidth, "Falloff Width", "Maximum distance used to calculate gradient in meters.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->DataElement(0, &ShapeAreaFalloffGradientConfig::m_falloffType, "Falloff Type", "Function for calculating falloff")

                    // Inner falloff isn't supported yet.
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                    //->EnumAttribute(ShapeAreaFalloffGradientConfig::FalloffType::Inner, "Inner")
                    ->EnumAttribute(FalloffType::Outer, "Outer")
                    //->EnumAttribute(ShapeAreaFalloffGradientConfig::FalloffType::InnerOuter, "InnerOuter")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &ShapeAreaFalloffGradientConfig::m_is3dFalloff, "3D Falloff",
                        "Either calculate the falloff in the XY plane or in 3D space.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ShapeAreaFalloffGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("shapeEntityId", BehaviorValueProperty(&ShapeAreaFalloffGradientConfig::m_shapeEntityId))
                ->Property("falloffWidth", BehaviorValueProperty(&ShapeAreaFalloffGradientConfig::m_falloffWidth))
                ->Property("falloffType",
                    [](ShapeAreaFalloffGradientConfig* config) { return (AZ::u8&)(config->m_falloffType); },
                    [](ShapeAreaFalloffGradientConfig* config, const AZ::u8& i) { config->m_falloffType = (FalloffType)i; })
                ->Property("is3dFalloff", BehaviorValueProperty(&ShapeAreaFalloffGradientConfig::m_is3dFalloff))
                ;
        }
    }

    void ShapeAreaFalloffGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void ShapeAreaFalloffGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void ShapeAreaFalloffGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void ShapeAreaFalloffGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        ShapeAreaFalloffGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ShapeAreaFalloffGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ShapeAreaFalloffGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ShapeAreaFalloffGradientComponentTypeId", BehaviorConstant(ShapeAreaFalloffGradientComponentTypeId));

            behaviorContext->Class<ShapeAreaFalloffGradientComponent>()->RequestBus("ShapeAreaFalloffGradientRequestBus");

            behaviorContext->EBus<ShapeAreaFalloffGradientRequestBus>("ShapeAreaFalloffGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetShapeEntityId", &ShapeAreaFalloffGradientRequestBus::Events::GetShapeEntityId)
                ->Event("SetShapeEntityId", &ShapeAreaFalloffGradientRequestBus::Events::SetShapeEntityId)
                ->VirtualProperty("ShapeEntityId", "GetShapeEntityId", "SetShapeEntityId")
                ->Event("GetFalloffWidth", &ShapeAreaFalloffGradientRequestBus::Events::GetFalloffWidth)
                ->Event("SetFalloffWidth", &ShapeAreaFalloffGradientRequestBus::Events::SetFalloffWidth)
                ->VirtualProperty("FalloffWidth", "GetFalloffWidth", "SetFalloffWidth")
                ->Event("GetFalloffType", &ShapeAreaFalloffGradientRequestBus::Events::GetFalloffType)
                ->Event("SetFalloffType", &ShapeAreaFalloffGradientRequestBus::Events::SetFalloffType)
                ->VirtualProperty("FalloffType", "GetFalloffType", "SetFalloffType")
                ->Event("Get3dFalloff", &ShapeAreaFalloffGradientRequestBus::Events::Get3dFalloff)
                ->Event("Set3dFalloff", &ShapeAreaFalloffGradientRequestBus::Events::Set3dFalloff)
                ->VirtualProperty("Is3dFalloff", "Get3dFalloff", "Set3dFalloff");
        }
    }

    ShapeAreaFalloffGradientComponent::ShapeAreaFalloffGradientComponent(const ShapeAreaFalloffGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ShapeAreaFalloffGradientComponent::Activate()
    {
        ShapeAreaFalloffGradientRequestBus::Handler::BusConnect(GetEntityId());

        // Make sure we're notified whenever the shape changes, so that we can re-cache its center point.
        if (m_configuration.m_shapeEntityId.IsValid())
        {
            AZ::EntityBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
        }

        // Keep track of the center of the shape so that we can calculate falloff distance correctly.
        CacheShapeBounds();

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ShapeAreaFalloffGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        ShapeAreaFalloffGradientRequestBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
    }

    bool ShapeAreaFalloffGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ShapeAreaFalloffGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ShapeAreaFalloffGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ShapeAreaFalloffGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float ShapeAreaFalloffGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        float distance = 0.0f;
        AZ::Vector3 queryPoint = sampleParams.m_position;
        if (!m_configuration.m_is3dFalloff)
        {
            // Calculate the shape falloff distance in the XY plane only by using the shape center as our Z location.
            queryPoint.SetZ(m_cachedShapeCenter.GetZ());
        }

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceFromPoint, queryPoint);

        // Since this is outer falloff, distance should give us values from 1.0 at the minimum distance to 0.0 at the maximum distance.
        // The statement is written specifically to handle the 0 falloff case as well. For 0 falloff, all points inside the shape
        // (0 distance) return 1.0, and all points outside the shape return 0. This works because division by 0 gives infinity, which gets
        // clamped by the GetMax() to 0.  However, if distance == 0, it would give us NaN, so we have the separate conditional check to
        // handle that case and clamp to 1.0.
        return (distance <= 0.0f) ? 1.0f : AZ::GetMax(1.0f - (distance / m_configuration.m_falloffWidth), 0.0f);
    }

    void ShapeAreaFalloffGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        bool shapeConnected = false;
        const float falloffWidth = m_configuration.m_falloffWidth;

        LmbrCentral::ShapeComponentRequestsBus::Event(
            m_configuration.m_shapeEntityId,
            [this, falloffWidth, positions, &outValues, &shapeConnected](LmbrCentral::ShapeComponentRequestsBus::Events* shapeRequests)
            {
                shapeConnected = true;

                for (size_t index = 0; index < positions.size(); index++)
                {
                    AZ::Vector3 queryPoint = positions[index];
                    if (!m_configuration.m_is3dFalloff)
                    {
                        // Calculate the shape falloff distance in the XY plane only by using the shape center as our Z location.
                        queryPoint.SetZ(m_cachedShapeCenter.GetZ());
                    }

                    float distance = shapeRequests->DistanceFromPoint(queryPoint);

                    // Since this is outer falloff, distance should give us values from 1.0 at the minimum distance to 0.0 at the maximum
                    // distance. The statement is written specifically to handle the 0 falloff case as well. For 0 falloff, all points
                    // inside the shape (0 distance) return 1.0, and all points outside the shape return 0. This works because division by 0
                    // gives infinity, which gets clamped by the GetMax() to 0.  However, if distance == 0, it would give us NaN, so we have
                    // the separate conditional check to handle that case and clamp to 1.0.
                    outValues[index] = (distance <= 0.0f) ? 1.0f : AZ::GetMax(1.0f - (distance / falloffWidth), 0.0f);
                }
            });

        // If there's no shape, there's no falloff.
        if (!shapeConnected)
        {
            for (auto& outValue : outValues)
            {
                outValue = 1.0f;
            }
        }
    }

    void ShapeAreaFalloffGradientComponent::NotifyRegionChanged(const AZ::Aabb& region)
    {
        if (region.IsValid())
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, region);
        }
        else
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    AZ::EntityId ShapeAreaFalloffGradientComponent::GetShapeEntityId() const
    {
        return m_configuration.m_shapeEntityId;
    }

    void ShapeAreaFalloffGradientComponent::SetShapeEntityId(AZ::EntityId entityId)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);

            // If we're setting the entityId to the same one, don't do anything.
            if (entityId == m_configuration.m_shapeEntityId)
            {
                return;
            }

            m_configuration.m_shapeEntityId = entityId;

            AZ::EntityBus::Handler::BusDisconnect();
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
            if (m_configuration.m_shapeEntityId.IsValid())
            {
                AZ::EntityBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
                LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
            }
        }

        CacheShapeBounds();
    }

    float ShapeAreaFalloffGradientComponent::GetFalloffWidth() const
    {
        return m_configuration.m_falloffWidth;
    }

    void ShapeAreaFalloffGradientComponent::SetFalloffWidth(float falloffWidth)
    {
        AZ::Aabb dirtyRegion = AZ::Aabb::CreateNull();

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            // We only support outer falloff, so our dirty region is our shape expanded by the larger falloff width.
            dirtyRegion = m_cachedShapeBounds.GetExpanded(AZ::Vector3(AZ::GetMax(m_configuration.m_falloffWidth, falloffWidth)));
            m_configuration.m_falloffWidth = falloffWidth;
        }

        NotifyRegionChanged(dirtyRegion);
    }

    FalloffType ShapeAreaFalloffGradientComponent::GetFalloffType() const
    {
        return m_configuration.m_falloffType;
    }

    void ShapeAreaFalloffGradientComponent::SetFalloffType(FalloffType type)
    {
        AZ::Aabb dirtyRegion = AZ::Aabb::CreateNull();

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_falloffType = type;

            // We only support outer falloff, so our dirty region is our shape expanded by the falloff width.
            dirtyRegion = m_cachedShapeBounds.GetExpanded(AZ::Vector3(m_configuration.m_falloffWidth));
        }

        NotifyRegionChanged(dirtyRegion);
    }

    bool ShapeAreaFalloffGradientComponent::Get3dFalloff() const
    {
        return m_configuration.m_is3dFalloff;
    }

    void ShapeAreaFalloffGradientComponent::Set3dFalloff(bool is3dFalloff)
    {
        AZ::Aabb dirtyRegion = AZ::Aabb::CreateNull();

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_is3dFalloff = is3dFalloff;

            // We only support outer falloff, so our dirty region is our shape expanded by the falloff width.
            dirtyRegion = m_cachedShapeBounds.GetExpanded(AZ::Vector3(m_configuration.m_falloffWidth));
        }

        NotifyRegionChanged(dirtyRegion);
    }
    
    void ShapeAreaFalloffGradientComponent::OnShapeChanged(
        [[maybe_unused]] LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        CacheShapeBounds();
    }

    void ShapeAreaFalloffGradientComponent::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        CacheShapeBounds();
    }

    void ShapeAreaFalloffGradientComponent::OnEntityDeactivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        CacheShapeBounds();
    }

    void ShapeAreaFalloffGradientComponent::CacheShapeBounds()
    {
        AZ::Aabb dirtyRegion = AZ::Aabb::CreateNull();

        {
            AZStd::unique_lock lock(m_queryMutex);

            AZ::Aabb previousShapeBounds = m_cachedShapeBounds;

            m_cachedShapeBounds = AZ::Aabb::CreateNull();

            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                m_cachedShapeBounds, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            // Grab the center of the shape so that we can calculate falloff distance in 2D.
            if (m_cachedShapeBounds.IsValid())
            {
                m_cachedShapeCenter = m_cachedShapeBounds.GetCenter();
            }
            else
            {
                m_cachedShapeCenter = AZ::Vector3::CreateZero();
            }

            // Calculate the dirty region based on the previous and current shape bounds.
            // If either the previous or current shape bounds is invalid, then leave the dirtyRegion invalid.
            // This component returns 1.0 everywhere if there's no shape, because technically there's no falloff from max,
            // so changing to or from a valid shape will cause potential value changes across the entire world space.
            if (previousShapeBounds.IsValid() && m_cachedShapeBounds.IsValid())
            {
                dirtyRegion.AddAabb(previousShapeBounds.GetExpanded(AZ::Vector3(m_configuration.m_falloffWidth)));
                dirtyRegion.AddAabb(m_cachedShapeBounds.GetExpanded(AZ::Vector3(m_configuration.m_falloffWidth)));
            }
        }

        // Any time we're caching the shape bounds, it's presumably because the shape changed, so notify about the change.
        NotifyRegionChanged(dirtyRegion);
    }
} // namespace GradientSignal
