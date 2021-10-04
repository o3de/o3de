/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace Multiplayer
{
    class MultiplayerController;
    class NetworkEntityTracker;
    class NetBindComponent;

    //! @class ConstNetworkEntityHandle
    //! @brief This class provides a wrapping around handle ids.
    //! It is optimized to avoid using the hashmap lookup unless the hashmap has had an item removed.
    class ConstNetworkEntityHandle
    {
    public:

        //! Constructs a nullptr handle.
        ConstNetworkEntityHandle() = default;

        //! Constructs a ConstNetworkEntityHandle given an entity, an entity tracker
        //! @param entity        pointer to the entity to construct a ConstNetworkEntityHandle for
        //! @param entityTracker pointer to the entity tracker that tracks the entity
        //!                      can optionally be null in which case the entity tracker will be looked up
        ConstNetworkEntityHandle(AZ::Entity* entity, const NetworkEntityTracker* entityTracker = nullptr);

        ConstNetworkEntityHandle(const ConstNetworkEntityHandle&) = default;

        //! Access the AZ::Entity if it safely exists, nullptr or false is returned if the entity does not exist.
        //! @{
        bool Exists() const;
        AZ::Entity* GetEntity();
        const AZ::Entity* GetEntity() const;
        //! @}

        //! Operators providing pointer semantics.
        //! @{
        bool operator ==(const ConstNetworkEntityHandle& rhs) const;
        bool operator !=(const ConstNetworkEntityHandle& rhs) const;
        friend bool operator ==(const ConstNetworkEntityHandle& lhs, AZStd::nullptr_t);
        friend bool operator ==(AZStd::nullptr_t, const ConstNetworkEntityHandle& rhs);
        friend bool operator !=(const ConstNetworkEntityHandle& lhs, AZStd::nullptr_t);
        friend bool operator !=(AZStd::nullptr_t, const ConstNetworkEntityHandle& rhs);
        friend bool operator ==(const ConstNetworkEntityHandle& lhs, const AZ::Entity* rhs);
        friend bool operator ==(const AZ::Entity* lhs, const ConstNetworkEntityHandle& rhs);
        friend bool operator !=(const ConstNetworkEntityHandle& lhs, const AZ::Entity* rhs);
        friend bool operator !=(const AZ::Entity* lhs, const ConstNetworkEntityHandle& rhs);
        //! @}

        bool operator <(const ConstNetworkEntityHandle& rhs) const;

        explicit operator bool() const;

        //! Resets the handle to a nullptr state.
        void Reset();
        void Reset(const ConstNetworkEntityHandle& handle);

        //! Returns the networkEntityId of the entity this handle points to.
        //! @return the networkEntityId of the entity this handle points to
        NetEntityId GetNetEntityId() const;

        //! Returns the cached netBindComponent for this entity, or nullptr if it doesn't exist.
        //! @return the cached netBindComponent for this entity, or nullptr if it doesn't exist
        NetBindComponent* GetNetBindComponent() const;

        //! Returns a specific component on of entity given a typeId.
        //! @param typeId the typeId of the component to find and return
        //! @return pointer to the requested component, or nullptr if it doesn't exist on the entity
        const AZ::Component* FindComponent(const AZ::TypeId& typeId) const;

        //! Returns a specific component on of entity by class type.
        //! @return pointer to the requested component, or nullptr if it doesn't exist on the entity
        template <typename Component>
        const Component* FindComponent() const;

        //! Helper function for sorting EntityHandles by netEntityId.
        static bool Compare(const ConstNetworkEntityHandle& lhs, const ConstNetworkEntityHandle& rhs);

    protected:

        mutable uint32_t m_changeDirty = 0; // Optimization so we don't need to recheck the hashmap
        mutable AZ::Entity* m_entity = nullptr;
        mutable NetBindComponent* m_netBindComponent = nullptr;
        const NetworkEntityTracker* m_networkEntityTracker = nullptr;
        NetEntityId m_netEntityId = InvalidNetEntityId;
    };

    class NetworkEntityHandle
        : public ConstNetworkEntityHandle
    {
    public:

        using ConstNetworkEntityHandle::ConstNetworkEntityHandle;

        //! Initializes the underlying entity if possible.
        void Init();

        //! Activates the underlying entity if possible.
        void Activate();

        //! Deactivates the underlying entity if possible.
        void Deactivate();

        //! Gets the BaseController from the first component on an Entity with the supplied typeId AND which inherits from Multiplayer::BaseComponent
        MultiplayerController* FindController(const AZ::TypeId& typeId);

        template <typename Controller>
        Controller* FindController();

        using ConstNetworkEntityHandle::FindComponent;
        AZ::Component* FindComponent(const AZ::TypeId& typeId);

        template <typename ComponentType>
        ComponentType* FindComponent();
    };
}

#include <Multiplayer/NetworkEntity/NetworkEntityHandle.inl>
