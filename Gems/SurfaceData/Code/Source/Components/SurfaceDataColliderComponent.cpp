/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/Components/SurfaceDataColliderComponent.h>

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
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
        services.push_back(AZ_CRC_CE("SurfaceDataModifierService"));
    }

    void SurfaceDataColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
        services.push_back(AZ_CRC_CE("SurfaceDataModifierService"));
    }

    void SurfaceDataColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("PhysicsColliderService"));
    }

    void SurfaceDataColliderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
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
        m_newPointWeights.AssignSurfaceTagWeights(m_configuration.m_providerTags, 1.0f);
        UpdateColliderData();
    }

    void SurfaceDataColliderComponent::Deactivate()
    {
        if (m_providerHandle != InvalidSurfaceDataRegistryHandle)
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            m_providerHandle = InvalidSurfaceDataRegistryHandle;
        }
        if (m_modifierHandle != InvalidSurfaceDataRegistryHandle)
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataModifier(m_modifierHandle);
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
            AZStd::unique_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
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

    void SurfaceDataColliderComponent::GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const
    {
        GetSurfacePointsFromList(AZStd::span<const AZ::Vector3>(&inPosition, 1), surfacePointList);
    }

    void SurfaceDataColliderComponent::GetSurfacePointsFromList(
        AZStd::span<const AZ::Vector3> inPositions, SurfacePointList& surfacePointList) const
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Event(
            GetEntityId(),
            [this, inPositions, &surfacePointList](AzPhysics::SimulatedBodyComponentRequestsBus::Events* simBody)
            {
                AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

                AzPhysics::RayCastRequest request;
                request.m_direction = -AZ::Vector3::CreateAxisZ();

                for (auto& inPosition : inPositions)
                {
                    // test AABB as first pass to claim the point
                    if (SurfaceData::AabbContains2D(m_colliderBounds, inPosition))
                    {
                        // We're casting the ray to look for a collision, so start at the top of the collider and cast downwards
                        // the full height of the collider.
                        request.m_start = AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_colliderBounds.GetMax().GetZ());
                        request.m_distance = m_colliderBounds.GetExtents().GetZ();

                        AzPhysics::SceneQueryHit result = simBody->RayCast(request);

                        if (result)
                        {
                            surfacePointList.AddSurfacePoint(
                                GetEntityId(), inPosition, result.m_position, result.m_normal, m_newPointWeights);
                        }
                    }
                }

            });
    }

    void SurfaceDataColliderComponent::ModifySurfacePoints(
            AZStd::span<const AZ::Vector3> positions,
            AZStd::span<const AZ::EntityId> creatorEntityIds,
            AZStd::span<SurfaceData::SurfaceTagWeights> weights) const
    {
        AZ_Assert(
            (positions.size() == creatorEntityIds.size()) && (positions.size() == weights.size()),
            "Sizes of the passed-in spans don't match");

        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

        // If we don't have a valid volume or don't have any modifier tags, there's nothing to do.
        if (!m_colliderBounds.IsValid() || m_configuration.m_modifierTags.empty())
        {
            return;
        }

        AzPhysics::SimulatedBodyComponentRequestsBus::Event(
            GetEntityId(),
            [this, positions, creatorEntityIds, &weights](AzPhysics::SimulatedBodyComponentRequestsBus::Events* simBody)
            {
                // We're checking to see if each point is inside the body, so we can initialize direction and distance for every check.
                // The direction shouldn't matter and the distance needs to be 0.
                AzPhysics::RayCastRequest request;
                request.m_direction = AZ::Vector3::CreateAxisZ();
                request.m_distance = 0.0f;

                AzPhysics::SceneQueryHit result;

                for (size_t index = 0; index < positions.size(); index++)
                {
                    // Only modify points that weren't created by this entity.
                    if (creatorEntityIds[index] != GetEntityId())
                    {
                        // Do a quick bounds check before performing the more expensive raycast.
                        if (m_colliderBounds.Contains(positions[index]))
                        {
                            // We're in bounds, so if the raycast succeeds too, then we're inside the volume.
                            request.m_start = positions[index];
                            result = simBody->RayCast(request);
                            if (result)
                            {
                                // If the query point collides with the volume, add all our modifier tags with a weight of 1.0f.
                                weights[index].AddSurfaceTagWeights(m_configuration.m_modifierTags, 1.0f);
                            }
                        }
                    }
                }
            });
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
        AZ_PROFILE_FUNCTION(SurfaceData);

        bool colliderValidBeforeUpdate = false;
        bool colliderValidAfterUpdate = false;

        {
            AZStd::unique_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

            colliderValidBeforeUpdate = m_colliderBounds.IsValid();

            m_colliderBounds = AZ::Aabb::CreateNull();
            AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(m_colliderBounds, GetEntityId(), &AzPhysics::SimulatedBodyComponentRequestsBus::Events::GetAabb);

            colliderValidAfterUpdate = m_colliderBounds.IsValid();
        }

        SurfaceDataRegistryEntry providerRegistryEntry;
        providerRegistryEntry.m_entityId = GetEntityId();
        providerRegistryEntry.m_bounds = m_colliderBounds;
        providerRegistryEntry.m_tags = m_configuration.m_providerTags;
        providerRegistryEntry.m_maxPointsCreatedPerInput = 1;

        SurfaceDataRegistryEntry modifierRegistryEntry(providerRegistryEntry);
        modifierRegistryEntry.m_tags = m_configuration.m_modifierTags;
        modifierRegistryEntry.m_maxPointsCreatedPerInput = 0;

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
            m_providerHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataProvider(providerRegistryEntry);
            m_modifierHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataModifier(modifierRegistryEntry);

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
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataModifier(m_modifierHandle);
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
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataProvider(m_providerHandle, providerRegistryEntry);
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataModifier(m_modifierHandle, modifierRegistryEntry);
        }
    }
}
