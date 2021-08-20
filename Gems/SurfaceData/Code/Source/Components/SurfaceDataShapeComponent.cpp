/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceDataShapeComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace SurfaceData
{
    void SurfaceDataShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataShapeConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ProviderTags", &SurfaceDataShapeConfig::m_providerTags)
                ->Field("ModifierTags", &SurfaceDataShapeConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceDataShapeConfig>(
                    "Shape Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceDataShapeConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &SurfaceDataShapeConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }
    }

    void SurfaceDataShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void SurfaceDataShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void SurfaceDataShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void SurfaceDataShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataShapeConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataShapeComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataShapeComponent::m_configuration)
                ;
        }
    }

    SurfaceDataShapeComponent::SurfaceDataShapeComponent(const SurfaceDataShapeConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceDataShapeComponent::Activate()
    {
        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        m_modifierHandle = InvalidSurfaceDataRegistryHandle;
        m_refresh = false;

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        // Update the cached shape data and bounds, then register the surface data provider / modifier
        UpdateShapeData();
    }

    void SurfaceDataShapeComponent::Deactivate()
    {
        if (m_providerHandle != InvalidSurfaceDataRegistryHandle)
        {
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
        }
        if (m_modifierHandle != InvalidSurfaceDataRegistryHandle)
        {
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
            m_modifierHandle = InvalidSurfaceDataRegistryHandle;

        }

        m_refresh = false;
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceDataModifierRequestBus::Handler::BusDisconnect();

        // Clear the cached shape data
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);
            m_shapeBounds = AZ::Aabb::CreateNull();
            m_shapeBoundsIsValid = false;
        }
    }

    bool SurfaceDataShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataShapeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataShapeConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void SurfaceDataShapeComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_shapeBoundsIsValid)
        {
            const AZ::Vector3 rayOrigin = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_shapeBounds.GetMax().GetZ());
            const AZ::Vector3 rayDirection = -AZ::Vector3::CreateAxisZ();
            float intersectionDistance = 0.0f;
            bool hitShape = false;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(hitShape, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, rayOrigin, rayDirection, intersectionDistance);
            if (hitShape)
            {
                SurfacePoint point;
                point.m_entityId = GetEntityId();
                point.m_position = rayOrigin + intersectionDistance * rayDirection;
                point.m_normal = AZ::Vector3::CreateAxisZ();
                AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
                surfacePointList.push_back(point);
            }
        }
    }

    void SurfaceDataShapeComponent::ModifySurfacePoints(SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_shapeBoundsIsValid && !m_configuration.m_modifierTags.empty())
        {
            const AZ::EntityId entityId = GetEntityId();
            for (auto& point : surfacePointList)
            {
                if (point.m_entityId != entityId && m_shapeBounds.Contains(point.m_position))
                {
                    bool inside = false;
                    LmbrCentral::ShapeComponentRequestsBus::EventResult(inside, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point.m_position);
                    if (inside)
                    {
                        AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                    }
                }
            }
        }
    }

    void SurfaceDataShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        OnCompositionChanged();
    }

    void SurfaceDataShapeComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        OnCompositionChanged();
    }

    void SurfaceDataShapeComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_refresh)
        {
            UpdateShapeData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SurfaceDataShapeComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SurfaceDataShapeComponent::UpdateShapeData()
    {
        AZ_PROFILE_FUNCTION(Entity);

        bool shapeValidBeforeUpdate = false;
        bool shapeValidAfterUpdate = false;

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            shapeValidBeforeUpdate = m_shapeBoundsIsValid;

            m_shapeBounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(m_shapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            m_shapeBoundsIsValid = m_shapeBounds.IsValid();

            shapeValidAfterUpdate = m_shapeBoundsIsValid;
        }

        SurfaceDataRegistryEntry providerRegistryEntry;
        providerRegistryEntry.m_entityId = GetEntityId();
        providerRegistryEntry.m_bounds = m_shapeBounds;
        providerRegistryEntry.m_tags = m_configuration.m_providerTags;

        SurfaceDataRegistryEntry modifierRegistryEntry(providerRegistryEntry);
        modifierRegistryEntry.m_tags = m_configuration.m_modifierTags;

        if (shapeValidBeforeUpdate && shapeValidAfterUpdate)
        {
            // Our shape was valid before and after, it just changed in some way, so update our registry entries
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid modifier data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, providerRegistryEntry);
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, modifierRegistryEntry);
        }
        else if (!shapeValidBeforeUpdate && shapeValidAfterUpdate)
        {
            // Our shape has become valid, so register as a provider and save off the registry handles
            AZ_Assert((m_providerHandle == InvalidSurfaceDataRegistryHandle), "Surface Provider data handle is initialized before our shape became valid");
            AZ_Assert((m_modifierHandle == InvalidSurfaceDataRegistryHandle), "Surface Modifier data handle is initialized before our shape became valid");
            SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, providerRegistryEntry);
            SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, modifierRegistryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
            SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
        }
        else if (shapeValidBeforeUpdate && !shapeValidAfterUpdate)
        {
            // Our shape has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
            m_modifierHandle = InvalidSurfaceDataRegistryHandle;

            SurfaceDataProviderRequestBus::Handler::BusDisconnect();
            SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        }
        else
        {
            // We didn't have a valid shape before or after running this, so do nothing.
        }
    }

    const float SurfaceDataShapeComponent::s_rayAABBHeightPadding = 0.1f;
}
