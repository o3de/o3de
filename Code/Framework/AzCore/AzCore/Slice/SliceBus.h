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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    class Entity;

    /**
     */
    class SliceEvents
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        virtual ~SliceEvents() {}
    };

    typedef EBus<SliceEvents> SliceBus;

    /**
    * Events dispatched during the creation, modification, and destruction of a slice instance
    */
    class SliceInstanceEvents
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        /// Called when a slice metadata entity has been destroyed.
        virtual void OnMetadataEntityCreated(const SliceComponent::SliceInstanceAddress& /*sliceAddress*/, AZ::Entity& /*entity*/) {}

        virtual void OnMetadataEntityDestroyed(AZ::EntityId /*entityId*/) {}
    };
    using SliceInstanceNotificationBus = AZ::EBus<SliceInstanceEvents>;

    /**
     * Dispatches events at key points of SliceAsset serialization.
     */
    class SliceAssetSerializationNotifications
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        /// Called after a slice is loaded from serialized data.
        /// This event can be used to "fix up" a loaded slice before it is used.
        /// If you wish to "fix up" entities, use OnSliceEntitiesLoaded instead.
        virtual void OnWriteDataToSliceAssetEnd(AZ::SliceComponent& /*slice*/) {}

        /// Called each time a slice loads entities from serialized data.
        /// This event can be used to "fix up" loaded entities before they are used anywhere else.
        /// Note that this event may be sent multiple times from a single slice:
        /// - for entities that are "new" to a slice, this event is sent as soon as the slice is loaded.
        /// - for "instanced" entities, this event is sent while the slice is being instantiated.
        virtual void OnSliceEntitiesLoaded(const AZStd::vector<AZ::Entity*>& /*entities*/) {}

        //! Fired at the very beginning of a slice push transaction
        virtual void OnBeginSlicePush(const AZ::Data::AssetId& /*sliceAssetId*/) {}

        //! Fired at the very end of a slice push transaction
        virtual void OnEndSlicePush(const AZ::Data::AssetId& /*originalSliceAssetId*/, const AZ::Data::AssetId& /*finalSliceAssetId*/) {}
    };
    using SliceAssetSerializationNotificationBus = AZ::EBus<SliceAssetSerializationNotifications>;


    /**
     * Interface for AZ::SliceEntityHierarchyRequestBus, which is an EBus 
     * that provides hierarchical entity information for use in slice tools.
     *
     * This is necessary because different types of entities can have different
     * ways of implementing who their parent or children are - for example, 
     * normal world entities use the TransformComponent for tracking parent/child
     * relationships, while UI entities use UiElementComponent (which is separate
     * from the 2D transform component for UI entities). For different entity types,
     * the component keeping parent/child hierarchy information should implement
     * the AZ::SliceEntityHierarchyRequestBus so things like Slice Save widget and
     * SliceTransactions work.
     *
     * Hierarchy information is used for internal logic like "if I'm adding an entity to a
     * slice, its parent also needs to be added/already in the slice".
     */
    class SliceEntityHierarchyInterface
        : public ComponentBus
    {
    public:
        AZ_RTTI(SliceEntityHierarchyInterface, "{E5D09F26-217F-4182-8B1D-BB08E4859F12}");

        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

        /**
         * Destroys the instance of the class.
         */
        virtual ~SliceEntityHierarchyInterface() {}


        /**
         * Returns the entity ID of the entity's parent.
         * @return The entity ID of the parent. The entity ID is invalid if the
         * entity does not have a parent with a valid entity ID.
         */
        virtual EntityId GetSliceEntityParentId() { return EntityId(); }

        /**
         * Returns the entity IDs of the entity's immediate children.
         * @return A vector that contains the entity IDs of the entity's immediate children.
         */
        virtual AZStd::vector<AZ::EntityId> GetSliceEntityChildren() { return AZStd::vector<AZ::EntityId>(); };

    };

    /**
     * The EBus for requests for the parent and children of an entity for the
     * purposes of slice dependency hierarchies.
     * The events are defined in the AZ:: class.
     */
    using SliceEntityHierarchyRequestBus = AZ::EBus<SliceEntityHierarchyInterface>;

    /// @deprecated Use SliceComponent.
    using PrefabComponent = SliceComponent;

    /// @deprecated Use SliceEvents.
    using PrefabEvents = SliceEvents;

    /// @deprecated Use SliceBus.
    using PrefabBus = SliceBus;
} // namespace AZ