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

#include <Source/EntityDomains/SpatialEntityDomain.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    AZ_CVAR(float, sv_SpatialEntityDomainWidth, 20.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "This is the area around the non-overlapping map region over which the server is willing to control entities. This makes it so that if an entity is walking across the MapRegion boundry back and forth, they won't ping pong between servers.");

    SpatialEntityDomain::SpatialEntityDomain(const AZ::Aabb& aabb)
        : m_aabb(aabb)
        , m_controllersActivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersActivated(entityHandle, entityIsMigrating); })
        , m_controllersDeactivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersDeactivated(entityHandle, entityIsMigrating); })
    {
        // Slightly expand our Aabb to avoid entities rapidly toggling back and forth between domains
        m_aabb.Expand(AZ::Vector3(sv_SpatialEntityDomainWidth, sv_SpatialEntityDomainWidth, sv_SpatialEntityDomainWidth));
    }

    bool SpatialEntityDomain::IsInDomain(const ConstNetworkEntityHandle& entityHandle) const
    {
        if (const AZ::Entity* entity = entityHandle.GetEntity())
        {
            const AZ::Transform transform = entity->GetTransform()->GetWorldTM();
            return IsTransformInDomain(transform);
        }
        return false;
    }

    void SpatialEntityDomain::ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet)
    {
        for (auto& entityHandle : ownedEntitySet)
        {
            OnControllersActivated(entityHandle, EntityIsMigrating::False);
        }
        GetNetworkEntityManager()->AddControllersActivatedHandler(m_controllersActivatedHandler);
        GetNetworkEntityManager()->AddControllersDeactivatedHandler(m_controllersDeactivatedHandler);
    }

    void SpatialEntityDomain::RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const
    {
        // validate all our entities did not come back into the domain
        for (ConstNetworkEntityHandle& entityHandle : m_dirtyEntities)
        {
            AZ::Entity* entity = entityHandle.GetEntity();

            // If the entity no longer exists, we can safely skip it
            if (entity == nullptr)
            {
                continue;
            }

            // Turn back on tracking, we need this if the entity is in or out of domain (since entities can walk back into our domain prior to migrating)
            AZ::TransformInterface* transformInterface = entity->GetTransform();
            auto locationDataIter = m_ownedEntities.find(entityHandle);
            AZ_Assert(locationDataIter != m_ownedEntities.end(), "This should always exist");
            transformInterface->BindTransformChangedEventHandler(locationDataIter->second.m_updateEventHandler);

            if (!IsInDomain(entityHandle))
            {
                m_entitiesNotInDomain.insert(entityHandle.GetNetEntityId());
            }
        }
        m_dirtyEntities.clear();
        outEntitiesNotInDomain.insert(m_entitiesNotInDomain.begin(), m_entitiesNotInDomain.end());
    }

    void SpatialEntityDomain::DebugDraw() const
    {
        static constexpr float   BoundaryStripeHeight = 1.0f;
        static constexpr float   BoundaryStripeSpacing = 0.5f;
        static constexpr int32_t BoundaryStripeCount = 10;

        //auto* loc = draw.GetOwnerConst()->FindComponent<LocationComponent::Server>();
        //if (loc == nullptr)
        //{
        //    return;
        //}
        //
        //Vec3 dmnMin = m_SpatialEntityDomainParams.GetMin();
        //Vec3 dmnMax = m_SpatialEntityDomainParams.GetMax();
        //
        //dmnMin.z = loc->GetPosition().z;
        //dmnMax.z = dmnMin.z + BoundaryStripeHeight;
        //
        //for (int i = 0; i < BoundaryStripeCount; ++i)
        //{
        //    draw.AABB(dmnMin, dmnMax, color);
        //    dmnMin.z += BoundaryStripeSpacing;
        //    dmnMax.z += BoundaryStripeSpacing;
        //}
    }

    const AZ::Aabb& SpatialEntityDomain::GetAabb() const
    {
        return m_aabb;
    }

    bool SpatialEntityDomain::IsTransformInDomain(const AZ::Transform& transform) const
    {
        return m_aabb.Contains(transform.GetTranslation());
    }

    void SpatialEntityDomain::EntityTransformUpdated(const ConstNetworkEntityHandle& entityHandle)
    {
        m_dirtyEntities.push_back(entityHandle);
        LocationData& locationData = m_ownedEntities[entityHandle];
        // we marked this entity as dirty, we don't need to be attached to the movement event anymore
        locationData.m_updateEventHandler.Disconnect();
    }

    void SpatialEntityDomain::OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        const AZ::Entity* entity = entityHandle.GetEntity();

        // If the entity no longer exists, we can safely skip it
        if (entity != nullptr)
        {
            LocationData& locationData = m_ownedEntities[entityHandle];
            locationData.m_parent = this;
            locationData.m_entityHandle = entityHandle;

            // Turn back on tracking, we need this if the entity is in or out of domain (since entities can walk back into our domain prior to migrating)
            AZ::TransformInterface* transformInterface = entity->GetTransform();
            transformInterface->BindTransformChangedEventHandler(locationData.m_updateEventHandler);
        }

        if (!IsInDomain(entityHandle))
        {
            m_entitiesNotInDomain.insert(entityHandle.GetNetEntityId());
        }
    }

    void SpatialEntityDomain::OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_entitiesNotInDomain.erase(entityHandle.GetNetEntityId());
        m_ownedEntities.erase(entityHandle);
    }
}
