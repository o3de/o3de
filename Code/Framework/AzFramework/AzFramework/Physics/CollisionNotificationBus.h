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

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Physics/WorldEventhandler.h>

namespace Physics
{
    /// CollisionNotifications
    /// Bus interface for receiving collision events from a Physics::World
    /// 
    /// The bus is addressed by EntityId. Body1 inside collisionEvent will correspond
    /// to the eEntity id subscribed to. Body2 will always be the other body colliding with the entity.
    class CollisionNotifications
        : public AZ::ComponentBus
    {
    public:
        // Ebus Traits. ID'd on body1 entity Id
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const bool EnableEventQueue = true;
        using BusIdType = AZ::EntityId;

        virtual ~CollisionNotifications() {}

        /// Dispatched when two shapes start colliding.
        virtual void OnCollisionBegin(const CollisionEvent& /*collisionEvent*/) {}

        /// Dispatched when two shapes continue colliding.
        virtual void OnCollisionPersist(const CollisionEvent& /*collisionEvent*/) {}

        /// Dispatched when two shapes stop colliding.
        virtual void OnCollisionEnd(const CollisionEvent& /*collisionEvent*/) {}
    };

    /// Bus to service the PhysX Trigger Area Component event group.
    using CollisionNotificationBus = AZ::EBus<CollisionNotifications>;
} // namespace PhysX