/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>
#include <AzCore/base.h>

namespace LmbrCentral
{
    using Tag = AZ::Crc32;
    using Tags = AZStd::unordered_set<Tag>;

    // Provides services for querying Tags on entities.
    class TagGlobalRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = Tag;

        virtual ~TagGlobalRequests() = default;

        /// Handlers will respond if they have the tag (i.e. they are listening on the tag's channel)
        /// Use AZ::EbusAggregateResults to handle more than the first responder
        virtual const AZ::EntityId RequestTaggedEntities() { return AZ::EntityId(); }
    };
    using TagGlobalRequestBus = AZ::EBus<TagGlobalRequests>;

    /**
     * Use this bus if you want to know when the list of all entities with a given tag changes.
     * When you connect to this bus it will fire your handler once for each entity already with this tag
    **/
    class TagGlobalNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = Tag;
        virtual ~TagGlobalNotifications() = default;

        /**
         * When connecting to this bus, your OnEntityTagAdded handler will fire once for
         * each entity that already has this tag.
         * After initial connection you will be alerted when ever a new entity gains or loses the given tag
        **/
        virtual void OnEntityTagAdded(const AZ::EntityId&) = 0;

        /**
         * You will be alerted when ever an entity with a given tag has that tag removed
        **/
        virtual void OnEntityTagRemoved(const AZ::EntityId&) = 0;

        /**
         * This connection policy will cause the connecting handler to be fired once for each entity that already has the given tag
        **/
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                AZ::EBusAggregateResults<AZ::EntityId> results;
                TagGlobalRequestBus::EventResult(results, id, &TagGlobalRequestBus::Events::RequestTaggedEntities);
                for (const AZ::EntityId& entity : results.values)
                {
                    handler->OnEntityTagAdded(entity);
                }
            }
        };
    };
    using TagGlobalNotificationBus = AZ::EBus<TagGlobalNotifications>;

    // Provides services for managing Tags on entities.
    class TagComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual ~TagComponentRequests() = default;

        /// Returns true if the entity has the tag
        virtual bool HasTag(const Tag&) = 0;

        /// Adds the tag to the entity if it didn't already have it
        virtual void AddTag(const Tag&) = 0;

        /// Add a list of tags to the entity if it didn't already have them
        virtual void AddTags(const Tags& tags)  { for (const Tag& tag : tags) AddTag(tag); }

        /// Removes a tag from the entity if it had it
        virtual void RemoveTag(const Tag&) = 0;

        /// Removes a list of tags from the entity if it had them
        virtual void RemoveTags(const Tags& tags) { for (const Tag& tag : tags) RemoveTag(tag); }

        /// Gets the list of tags on the entity
        virtual const Tags& GetTags() { static Tags s_emptyTags; return s_emptyTags; }
    };
    using TagComponentRequestBus = AZ::EBus<TagComponentRequests>;

    // Notifications regarding Tags on entities.
    class TagComponentNotifications
        : public AZ::ComponentBus
    {
        public:
            //! Notifies listeners about tags being added
            virtual void OnTagAdded(const Tag&) {}

            //! Notifies listeners about tags being removed
            virtual void OnTagRemoved(const Tag&) {}
    };
    using TagComponentNotificationsBus = AZ::EBus<TagComponentNotifications>;

} // namespace LmbrCentral
