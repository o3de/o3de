/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class EntityId;
    class SliceMetadataInfoComponent;

    /**
    * Bus for making requests to the slice metadata context component.
    */
    class SliceMetadataInfoRequests
        : public ComponentBus
    {
    public:
        virtual ~SliceMetadataInfoRequests() {}

        /**
        * True if the entity ID is associated with the metadata entity.
        */
        virtual bool IsAssociated(EntityId entityId) = 0;

        /**
        * Fills out the provided container with all of the IDs of entities associated with this metadata entity.
        */
        virtual void GetAssociatedEntities(AZStd::set<EntityId>& /*associatedEntityIds*/) = 0;

        /**
        * Retrieve the Id of the parent
        */
        virtual EntityId GetParentId() = 0;

        /**
        * Fills out the provided container with all of the IDs of the metadata entities under this one in the slice hierarchy.
        */
        virtual void GetChildIDs(AZStd::unordered_set<EntityId>& /*childEntityIds*/) = 0;

        /**
        * Returns the total number of associated entities and children.
        */
        virtual size_t GetAssociationCount() = 0;
    };

    using SliceMetadataInfoRequestBus = EBus<SliceMetadataInfoRequests>;

    class SliceMetadataInfoManipulationRequests
        : public ComponentBus
    {
    public:
        /**
        * Add a child entity to the metadata entity.
        * Child entities must be metadata entities.
        */
        virtual void AddChildMetadataEntity(EntityId childEntityId) = 0;

        /**
        * Remove an existing child metadata entity.
        * The given entity must be a valid child metadata entity.
        */
        virtual void RemoveChildMetadataEntity(EntityId childEntityId) = 0;

        /**
        * Set the parent of the metadata entity.
        * Can be an invalid entity ID (No parent)
        * Must be the ID of a metadata entity.
        */
        virtual void SetParentMetadataEntity(EntityId parentEntityId) = 0;

        /**
        * Add an associated entity.
        * the associated entity must be an editor entity.
        */
        virtual void AddAssociatedEntity(EntityId associatedEntityId) = 0;

        /**
        * Fills out the provided container with all of the IDs of entities associated with this metadata entity.
        */
        virtual void GetAssociatedEntities(AZStd::set<EntityId>& /*associatedEntityIds*/) = 0;

        /**
        * Remove an entity from the association list.
        * The given entity must be an associated editor entity.
        */
        virtual void RemoveAssociatedEntity(EntityId associatedEntityId) = 0;

        /**
        * Marks this component as persistent.
        * When persistent, the component remains valid without children and associations.
        * For example, the root slice is persistent.
        */
        virtual void MarkAsPersistent(bool persistent) = 0;
    };

    using SliceMetadataInfoManipulationBus = EBus<SliceMetadataInfoManipulationRequests>;

    /**
    * Bus for receiving events/notifications when changes occur to the slice metadata information component.
    */
    class SliceMetadataInfoNotifications
        : public ComponentBus
    {
    public:
        virtual ~SliceMetadataInfoNotifications() {};

    public:
        /**
        * Sent when the metadata parent has been altered
        */
        virtual void OnMetadataParentChanged(EntityId /*newParentId*/) {}

        /**
        * Sent when the metadata has a new child added
        */
        virtual void OnChildMetadataEntityAdded(EntityId /*newChildId*/) {}

        /**
        * Sent when the metadata has a child removed
        */
        virtual void OnChildMetadataEntityRemoved(EntityId /*childId*/) {}

        /**
        * Sent when the metadata entity has no more dependencies (Associated entities or children)
        * This generally means that the slice attached to the metadata entity has been completely removed.
        * This notification should not be dispatched for components marked as persistent.
        * Note: This notification may trigger the destruction of the metadata entity and care should be
        * taken when dispatching it to that effect.
        */
        virtual void OnMetadataDependenciesRemoved() {}
    };

    using SliceMetadataInfoNotificationBus = EBus<SliceMetadataInfoNotifications>;
} // namespace AZ
