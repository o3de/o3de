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

#pragma once

#include <Source/EntityDomains/IEntityDomain.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>

namespace Multiplayer
{
    class MapRegion;
    struct SpatialEntityDomainParams;

    class SpatialEntityDomain
        : public IEntityDomain
    {
    public:
        SpatialEntityDomain(const AZ::Aabb& aabb);

        //! IEntityDomain overrides.
        //! @{
        bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const override;
        void ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet) override;
        void RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const override;
        void DebugDraw() const override;
        //! @}

        const AZ::Aabb& GetAabb() const;

    private:
        bool IsTransformInDomain(const AZ::Transform& transform) const;
        void EntityTransformUpdated(const ConstNetworkEntityHandle& entityHandle);

        void OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating);
        void OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating);

        struct LocationData
        {
            LocationData() = default;
            LocationData(LocationData&& rhs)
                : m_parent(rhs.m_parent)
                , m_entityHandle(rhs.m_entityHandle)
            {
                ;
            }

            SpatialEntityDomain* m_parent = nullptr;
            ConstNetworkEntityHandle m_entityHandle;
            AZ::TransformChangedEvent::Handler m_updateEventHandler = AZ::TransformChangedEvent::Handler
            (
                [this]([[maybe_unused]] const AZ::Transform& localTransform, [[maybe_unused]] const AZ::Transform& worldTransform)
                {
                    m_parent->EntityTransformUpdated(m_entityHandle);
                }
            );
        };

        AZ::Aabb m_aabb;

        // cached data
        mutable EntitiesNotInDomain m_entitiesNotInDomain;
        mutable AZStd::vector<ConstNetworkEntityHandle> m_dirtyEntities;
        mutable AZStd::unordered_map<ConstNetworkEntityHandle, LocationData> m_ownedEntities;
        ControllersActivatedEvent::Handler m_controllersActivatedHandler;
        ControllersDeactivatedEvent::Handler m_controllersDeactivatedHandler;
    };
}
