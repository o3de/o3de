/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    /**
     * Bus for making requests to the edit-time slice metadata context.
     *
     * The Slice Metadata Context creates and maintains entities whose components
     * store information associated with instantiated slices.
     */
    class SliceMetadataEntityContextRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~SliceMetadataEntityContextRequests() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /**
        * Retrieve the unique Id of the slice metadata context.
        */
        virtual AzFramework::EntityContextId GetSliceMetadataEntityContextId() = 0;

        /**
        * Resetting the context removes all entities from the context. Because the metadata entities themselves are owned by
        * their slices, this will not trigger destruction of the entities or remove them from the component application.
        */
        virtual void ResetContext() = 0;

        /**
        * Determines if an entity belongs to the Slice Metadata Entity Context
        * @param entityId And Entity to Check.
        */
        virtual bool IsSliceMetadataEntity(const AZ::EntityId entityId) = 0;

        /**
        * Get a raw pointer to an entity that belongs to this context
        * @param entityId The Id of the requested entity
        */
        virtual AZ::Entity* GetMetadataEntity(const AZ::EntityId entityId) = 0;

        /**
        * Get a list of the components required for entities in this context
        * @param required A component type list. The required component types will be appended to the end of it.
        */
        virtual void GetRequiredComponentTypes(AZ::ComponentTypeList& required) = 0;

        /**
        * Get the ID of the metadata entity associated with an editor entity
        * @return The ID of the associated entity, or an invalid ID if there is no association.
        */
        virtual AZ::EntityId GetMetadataEntityIdFromEditorEntity(const AZ::EntityId editorEntityId) = 0;

        /**
        * Get the ID of the metadata entity associated with a slice instance address
        * @param address The slice address of an instantiated slice
        * @return The ID of the associated entity, or an invalid ID if there is no association.
        */
        virtual AZ::EntityId GetMetadataEntityIdFromSliceAddress(const AZ::SliceComponent::SliceInstanceAddress& address) = 0;

        /**
        * Add a slice metadata entity to the context
        * @param sliceAddress The address of the slice associated with the metadata entity.
        # @param entity A pointer to the entity to add to the context
        */
        virtual void AddMetadataEntityToContext(const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, AZ::Entity& /*entity*/) = 0;
    };

    using SliceMetadataEntityContextRequestBus = AZ::EBus<SliceMetadataEntityContextRequests>;

    /**
     * Bus for receiving events/notifications from the slice metadata context.
     */
    class SliceMetadataEntityContextNotifications
        : public AZ::EBusTraits
    {
    public:

        virtual ~SliceMetadataEntityContextNotifications() {};

        /**
        * Dispatched when the context is reset
        */
        virtual void OnContextReset() {}

        /**
        * Dispatched when a metadata entity is added to the context
        */
        virtual void OnMetadataEntityAdded(AZ::EntityId /*entityId*/) {}

        /**
        * Dispatched when a metadata entity is removed from the context
        */
        virtual void OnMetadataEntityRemoved(AZ::EntityId /*entityId*/) {}
    };

    using SliceMetadataEntityContextNotificationBus = AZ::EBus<SliceMetadataEntityContextNotifications>;
} // namespace AzToolsFramework
