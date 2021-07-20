/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

namespace Physics
{
    //! CollisionRequests configures global project-level collision filtering settings.
    //! This is equivalent to setting values via the UI.
    class CollisionRequests
    {
    public:

        AZ_TYPE_INFO(CollisionRequests, "{5A937391-DC65-4E1D-84A6-AE151A1200D1}");

        CollisionRequests() = default;
        virtual ~CollisionRequests() = default;

        // AZ::Interface requires these to be deleted.
        CollisionRequests(CollisionRequests&&) = delete;
        CollisionRequests& operator=(CollisionRequests&&) = delete;

        /// Gets a collision layer by name. The Default layer is returned if the layer name was not found.
        virtual AzPhysics::CollisionLayer GetCollisionLayerByName(const AZStd::string& layerName) = 0;

        /// Looks up the name of a collision layer
        virtual AZStd::string GetCollisionLayerName(const AzPhysics::CollisionLayer& layer) = 0;

        /// Tries to find a collision layer which matches layerName. 
        /// Returns true if it was found and the result is stored in collisionLayer, otherwise false. 
        virtual bool TryGetCollisionLayerByName(const AZStd::string& layerName, AzPhysics::CollisionLayer& collisionLayer) = 0;

        /// Gets a collision group by name. The All group is returned if the group name was not found.
        virtual AzPhysics::CollisionGroup GetCollisionGroupByName(const AZStd::string& groupName) = 0;

        /// Tries to find a collision group which matches groupName. 
        /// Returns true if it was found, and the group is stored in collisionGroup, otherwise false.
        virtual bool TryGetCollisionGroupByName(const AZStd::string& groupName, AzPhysics::CollisionGroup& collisionGroup) = 0;

        /// Looks up a name from a collision group
        virtual AZStd::string GetCollisionGroupName(const AzPhysics::CollisionGroup& collisionGroup) = 0;

        /// Gets a collision group by id.
        virtual AzPhysics::CollisionGroup GetCollisionGroupById(const AzPhysics::CollisionGroups::Id& groupId) = 0;

        /// Sets the layer name by index.
        virtual void SetCollisionLayerName(int index, const AZStd::string& layerName) = 0;

        /// Creates a new collision group preset with corresponding groupName.
        virtual void CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group) = 0;
    };

    /// Collision requests bus traits. Singleton pattern.
    class CollisionRequestsTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using CollisionRequestBus = AZ::EBus<CollisionRequests, CollisionRequestsTraits>;

    //! CollisionFilteringRequests configures filtering settings per entity.
    class CollisionFilteringRequests
        : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        //! Sets the collision layer on an entity.
        //! layerName should match a layer defined in the PhysX cConfiguration window.
        //! Colliders with a matching colliderTag will be updated. Specify the empty tag to update all colliders.
        virtual void SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag) = 0;

        //! Gets the collision layer name for a collider on an entity
        //! If the collision layer can't be found, an empty string is returned.
        //! Note: Multiple colliders on an entity are currently not supported.
        virtual AZStd::string GetCollisionLayerName() = 0;

        //! Sets the collision group on an entity.
        //! groupName should match a group defined in the PhysX configuration window.
        //! Colliders with a matching colliderTag will be updated. Specify the empty tag to update all colliders.
        virtual void SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag) = 0;

        //! Gets the collision group name for a collider on an entity. 
        //! If the collision group can't be found, an empty string is returned.
        //! Note: Multiple colliders on an entity are currently not supported.
        virtual AZStd::string GetCollisionGroupName() = 0;

        //! Toggles a single collision layer on or off on an entity.
        //! layerName should match a layer defined in the PhysX configuration window.
        //! Colliders with a matching colliderTag will be updated. Specify the empty tag to update all colliders.
        virtual void ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled) = 0;
    };
    using CollisionFilteringRequestBus = AZ::EBus<CollisionFilteringRequests>;

} // namespace Physics
