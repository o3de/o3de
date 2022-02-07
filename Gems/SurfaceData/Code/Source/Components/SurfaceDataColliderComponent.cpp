/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceDataColliderComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/IntersectSegment.h>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>


namespace SurfaceData
{
    void SurfaceDataColliderConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SurfaceDataColliderConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ProviderTags", &SurfaceDataColliderConfig::m_providerTags)
                ->Field("ModifierTags", &SurfaceDataColliderConfig::m_modifierTags)
                ;

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<SurfaceDataColliderConfig>(
                    "PhysX Collider Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceDataColliderConfig::m_providerTags, "Generated Tags", "Surface tags to add to created points")
                    ->DataElement(0, &SurfaceDataColliderConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceDataColliderConfig>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Constructor()
                ->Property("providerTags", BehaviorValueProperty(&SurfaceDataColliderConfig::m_providerTags))
                ->Property("modifierTags", BehaviorValueProperty(&SurfaceDataColliderConfig::m_modifierTags))
                ;
        }
    }

    void SurfaceDataColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void SurfaceDataColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void SurfaceDataColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
    }

    void SurfaceDataColliderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("PhysicsWorldBodyService", 0x944da0cc));
    }

    void SurfaceDataColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataColliderConfig::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SurfaceDataColliderComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataColliderComponent::m_configuration)
                ;
        }
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceDataColliderComponent>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Constructor()
                ->Property("providerTags",
                    [](SurfaceDataColliderComponent* component) { return component->m_configuration.m_providerTags; },
                    [](SurfaceDataColliderComponent* component, SurfaceData::SurfaceTagVector value)
                       {
                            component->m_configuration.m_providerTags = value;
                            component->OnCompositionChanged();
                       })
                ->Property("modifierTags",
                    [](SurfaceDataColliderComponent* component) { return component->m_configuration.m_modifierTags; },
                    [](SurfaceDataColliderComponent* component, SurfaceData::SurfaceTagVector value)
                       {
                           component->m_configuration.m_modifierTags = value;
                           component->OnCompositionChanged();
                       })
                ;
        }
    }

    SurfaceDataColliderComponent::SurfaceDataColliderComponent(const SurfaceDataColliderConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceDataColliderComponent::Activate()
    {
        m_providerHandle = InvalidSurfaceDataRegistryHandle;
        m_modifierHandle = InvalidSurfaceDataRegistryHandle;
        m_refresh = false;

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Physics::ColliderComponentEventBus::Handler::BusConnect(GetEntityId());

        // Update the cached collider data and bounds, then register the surface data provider / modifier
        UpdateColliderData();
    }

    void SurfaceDataColliderComponent::Deactivate()
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

        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        Physics::ColliderComponentEventBus::Handler::BusDisconnect();
        SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        m_refresh = false;

        // Clear the cached mesh data
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);
            m_colliderBounds = AZ::Aabb::CreateNull();
        }
    }

    bool SurfaceDataColliderComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataColliderConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataColliderComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataColliderConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    bool SurfaceDataColliderComponent::DoRayTrace(const AZ::Vector3& inPosition, bool queryPointOnly, AZ::Vector3& outPosition, AZ::Vector3& outNormal) const
    {
        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        // test AABB as first pass to claim the point
        const AZ::Vector3 testPosition = AZ::Vector3(
            inPosition.GetX(),
            inPosition.GetY(),
            m_colliderBounds.GetCenter().GetZ());

        if (!m_colliderBounds.Contains(testPosition))
        {
            return false;
        }

        AzPhysics::RayCastRequest request;
        request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
        if (queryPointOnly)
        {
            // We're checking to see if the point is *inside* the collider, so give it a distance of 0.
            request.m_start = inPosition;
            request.m_distance = 0.0f;
        }
        else
        {
            // We're casting the ray to look for a collision, so start at the top of the collider and cast downwards
            // the full height of the collider.
            request.m_start = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_colliderBounds.GetMax().GetZ());
            request.m_distance = m_colliderBounds.GetExtents().GetZ();
        }

        AzPhysics::SceneQueryHit result;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(result, GetEntityId(), &AzPhysics::SimulatedBodyComponentRequestsBus::Events::RayCast, request);

        if (result)
        {
            outPosition = result.m_position;
            outNormal = result.m_normal;
            return true;
        }

        return false;
    }

    void SurfaceDataColliderComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        AZ::Vector3 hitPosition;
        AZ::Vector3 hitNormal;

        // We want a full raycast, so don't just query the start point.
        constexpr bool queryPointOnly = false;

        if (DoRayTrace(inPosition, queryPointOnly, hitPosition, hitNormal))
        {
            SurfacePoint point;
            point.m_entityId = GetEntityId();
            point.m_position = hitPosition;
            point.m_normal = hitNormal;
            AddMaxValueForMasks(point.m_masks, m_configuration.m_providerTags, 1.0f);
            surfacePointList.push_back(point);
        }
    }

    void SurfaceDataColliderComponent::ModifySurfacePoints(SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        if (m_colliderBounds.IsValid() && !m_configuration.m_modifierTags.empty())
        {
            const AZ::EntityId entityId = GetEntityId();
            for (auto& point : surfacePointList)
            {
                if (point.m_entityId != entityId && m_colliderBounds.Contains(point.m_position))
                {
                    AZ::Vector3 hitPosition;
                    AZ::Vector3 hitNormal;
                    constexpr bool queryPointOnly = true;
                    if (DoRayTrace(point.m_position, queryPointOnly, hitPosition, hitNormal))
                    {
                        AddMaxValueForMasks(point.m_masks, m_configuration.m_modifierTags, 1.0f);
                    }
                }
            }
        }
    }

    void SurfaceDataColliderComponent::OnCompositionChanged()
    {
        if (!m_refresh)
        {
            m_refresh = true;
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SurfaceDataColliderComponent::OnColliderChanged()
    {
        OnCompositionChanged();
    }

    void SurfaceDataColliderComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        OnCompositionChanged();
    }

    void SurfaceDataColliderComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_refresh)
        {
            UpdateColliderData();
            m_refresh = false;
        }
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SurfaceDataColliderComponent::UpdateColliderData()
    {
        AZ_PROFILE_FUNCTION(Entity);

        bool colliderValidBeforeUpdate = false;
        bool colliderValidAfterUpdate = false;

        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            colliderValidBeforeUpdate = m_colliderBounds.IsValid();

            m_colliderBounds = AZ::Aabb::CreateNull();
            AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(m_colliderBounds, GetEntityId(), &AzPhysics::SimulatedBodyComponentRequestsBus::Events::GetAabb);

            colliderValidAfterUpdate = m_colliderBounds.IsValid();
        }

        SurfaceDataRegistryEntry providerRegistryEntry;
        providerRegistryEntry.m_entityId = GetEntityId();
        providerRegistryEntry.m_bounds = m_colliderBounds;
        providerRegistryEntry.m_tags = m_configuration.m_providerTags;

        SurfaceDataRegistryEntry modifierRegistryEntry(providerRegistryEntry);
        modifierRegistryEntry.m_tags = m_configuration.m_modifierTags;


        if (!colliderValidBeforeUpdate && !colliderValidAfterUpdate)
        {
            // We didn't have a valid collider before or after running this, so do nothing.
        }
        else if (!colliderValidBeforeUpdate && colliderValidAfterUpdate)
        {
            // Our collider has become valid, so register as a provider and save off the provider handle
            AZ_Assert((m_providerHandle == InvalidSurfaceDataRegistryHandle), "Surface data handle is initialized before our collider became valid");
            AZ_Assert((m_modifierHandle == InvalidSurfaceDataRegistryHandle), "Surface Modifier data handle is initialized before our collider became valid");
            AZ_Assert(m_colliderBounds.IsValid(), "Collider Geometry isn't correctly initialized.");
            SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, providerRegistryEntry);
            SurfaceDataSystemRequestBus::BroadcastResult(m_modifierHandle, &SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, modifierRegistryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
            SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
        }
        else if (colliderValidBeforeUpdate && !colliderValidAfterUpdate)
        {
            // Our collider has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
            m_modifierHandle = InvalidSurfaceDataRegistryHandle;

            SurfaceDataProviderRequestBus::Handler::BusDisconnect();
            SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        }
        else if (colliderValidBeforeUpdate && colliderValidAfterUpdate)
        {
            // Our collider was valid before and after, it just changed in some way, so update our registry entry.
            AZ_Assert((m_providerHandle != InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ_Assert((m_modifierHandle != InvalidSurfaceDataRegistryHandle), "Invalid modifier data handle");
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataProvider, m_providerHandle, providerRegistryEntry);
            SurfaceDataSystemRequestBus::Broadcast(&SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier, m_modifierHandle, modifierRegistryEntry);
        }
    }
}
