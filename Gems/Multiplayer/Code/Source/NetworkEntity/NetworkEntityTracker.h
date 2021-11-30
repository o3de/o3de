/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/MultiplayerTypes.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Component/Entity.h>

namespace Multiplayer
{
    class NetBindComponent;

    //! @class NetworkEntityTracker
    //! @brief The responsibly of this class is to allow entity netEntityId's to be looked up.
    class NetworkEntityTracker
    {
    public:

        using EntityMap = AZStd::unordered_map<NetEntityId, AZ::Entity*>;
        using NetEntityIdMap = AZStd::unordered_map<AZ::EntityId, NetEntityId>;
        using NetBindingMap = AZStd::unordered_map<AZ::Entity*, NetBindComponent*>;
        using iterator = EntityMap::iterator;
        using const_iterator = EntityMap::const_iterator;

        NetworkEntityTracker() = default;

        //! Adds a networked entity to the tracker.
        //! @param netEntityId the networkId of the entity to add
        //! @param entity pointer to the entity corresponding to the networkId
        void Add(NetEntityId netEntityId, AZ::Entity* entity);

        //! Registers a new NetBindComponent with the NetworkEntityTracker.
        //! @param entity    pointer to the entity we are registering the NetBindComponent for
        //! @param component pointer to the NetBindComponent being registered
        void RegisterNetBindComponent(AZ::Entity* entity, NetBindComponent* component);

        //! Unregisters a NetBindComponent from the NetworkEntityTracker.
        //! @param component pointer to the NetBindComponent being removed
        void UnregisterNetBindComponent(NetBindComponent* component);

        //! Returns an entity handle which can validate entity existence.
        NetworkEntityHandle Get(NetEntityId netEntityId);
        ConstNetworkEntityHandle Get(NetEntityId netEntityId) const;

        //! Returns Net Entity ID for a given AZ Entity ID.
        NetEntityId Get(const AZ::EntityId& entityId) const;

        //! Returns true if the netEntityId exists.
        bool Exists(NetEntityId netEntityId) const;

        //! Get a raw pointer of an entity.
        AZ::Entity *GetRaw(NetEntityId netEntityId) const;

        //! Moves the given iterator out of the entity holder and returns the ptr.
        AZ::Entity *Move(EntityMap::iterator iter);

        //! Retrieves the NetBindComponent for the provided AZ::Entity, nullptr if the entity does not have netbinding.
        //! @param entity pointer to the entity to retrieve the NetBindComponent for
        //! @return pointer to the entities NetBindComponent, or nullptr if the entity doesn't exist or does not have netbinding
        NetBindComponent* GetNetBindComponent(AZ::Entity* rawEntity) const;

        //! Container overloads
        //!@{
        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;
        iterator find(NetEntityId netEntityId);
        const_iterator find(NetEntityId netEntityId) const;
        void erase(NetEntityId netEntityId);
        iterator erase(EntityMap::iterator iter);
        AZStd::size_t size() const;
        void clear();
        //! @}

        //! Dirty tracking optimizations to avoid unnecessary hash lookups.
        //! There are two counts, one for adds and one for deletes
        //! If an entity is nullptr, check adds to check to see if our entity was added again
        //! If an entity is not nullptr, check removes which reminds us to see if the entity no longer exists
        //! Passing in the entity into this helper assists in retrieving the correct count, so we do not need to store both counts inside each handle
        uint32_t GetChangeDirty(const AZ::Entity* entity) const;
        uint32_t GetDeleteChangeDirty() const;
        uint32_t GetAddChangeDirty() const;

        //! Prevent copying and heap allocation.
        AZ_DISABLE_COPY_MOVE(NetworkEntityTracker);

    private:

        EntityMap m_entityMap;
        NetEntityIdMap m_netEntityIdMap;
        NetBindingMap m_netBindingMap;
        uint32_t m_deleteChangeDirty = 0;
        uint32_t m_addChangeDirty = 0;
    };
}

#include "Source/NetworkEntity/NetworkEntityTracker.inl"
