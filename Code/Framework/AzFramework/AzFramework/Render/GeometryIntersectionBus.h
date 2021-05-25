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

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>

namespace AzFramework
{
    namespace RenderGeometry
    {
        struct EntityIdAndContext
        {
            EntityIdAndContext(AZ::EntityId entityId, AzFramework::EntityContextId contextId)
                : m_entityId(entityId)
                , m_contextId(contextId)
            {
            }

            bool operator ==(const EntityIdAndContext& rhs) const
            {
                return m_entityId == rhs.m_entityId && m_contextId == rhs.m_contextId;
            }

            AZ::EntityId m_entityId;
            AzFramework::EntityContextId m_contextId;
        };

        //! Interface for intersection requests, implement this interface to make your component
        //! intersect with render geometry.
        class IntersectionRequests
            : public AZ::EBusTraits
        {
            //! Policy for notifying the Intersector bus of entities connected/disconnected to this EBus
            //! so it updates the internal data of the entities
            template<class Bus>
            struct IntersectionRequestsConnectionPolicy
            {
                using BusPtr = typename Bus::BusPtr;
                using Context = typename Bus::Context;
                using HandlerNode = typename Bus::HandlerNode;
                using BusIdType = typename Bus::BusIdType;
                using ConnectLockGuard = typename Bus::Context::ConnectLockGuard;

                static void Connect([[maybe_unused]] BusPtr& busPtr, [[maybe_unused]] Context& context,
                    [[maybe_unused]] HandlerNode& handler, [[maybe_unused]] ConnectLockGuard& lockGuard, const BusIdType& id)
                {
                    IntersectionRequests::OnConnect(id);
                }
                static void Disconnect([[maybe_unused]] Context& context, HandlerNode& handler, [[maybe_unused]] BusPtr& busPtr)
                {
                    IntersectionRequests::OnDisconnect(handler.GetBusId());
                }
            };

        public:
            template<typename Bus>
            using ConnectionPolicy = IntersectionRequestsConnectionPolicy<Bus>;

            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = EntityIdAndContext;

            // Intersection against this render geometry
            virtual RayResult RenderGeometryIntersect(const RayRequest& ray) = 0;

        private:
            inline static void OnConnect(const EntityIdAndContext& entityAndContext);
            inline static void OnDisconnect(const EntityIdAndContext& entityAndContext);
        };

        using IntersectionRequestBus = AZ::EBus<IntersectionRequests>;

        //! Interface for data notification of changes in entity's render geometry
        class IntersectionNotifications
            : public AZ::EBusTraits
        {
        public:
            //! Listen by EntityContext id
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = EntityContextId;

            virtual void OnEntityConnected(AZ::EntityId entityId) = 0;
            virtual void OnEntityDisconnected(AZ::EntityId entityId) = 0;
            virtual void OnGeometryChanged(AZ::EntityId entityId) = 0;
        };
        using IntersectionNotificationBus = AZ::EBus<IntersectionNotifications>;

        void IntersectionRequests::OnConnect(const EntityIdAndContext& entityAndContext)
        {
            IntersectionNotificationBus::Event(entityAndContext.m_contextId, &IntersectionNotifications::OnEntityConnected, entityAndContext.m_entityId);
        }

        void IntersectionRequests::OnDisconnect(const EntityIdAndContext& entityAndContext)
        {
            IntersectionNotificationBus::Event(entityAndContext.m_contextId, &IntersectionNotifications::OnEntityDisconnected, entityAndContext.m_entityId);
        }
    }
}

namespace AZStd
{
    template<>
    struct hash<AzFramework::RenderGeometry::EntityIdAndContext>
    {
        inline size_t operator()(const AzFramework::RenderGeometry::EntityIdAndContext& entityIdAndContext) const
        {
            AZStd::hash<AzFramework::EntityContextId> entityContextIdHasher;
            size_t retVal = entityContextIdHasher(entityIdAndContext.m_contextId);
            AZStd::hash_combine(retVal, entityIdAndContext.m_entityId);
            return retVal;
        }
    };
}
