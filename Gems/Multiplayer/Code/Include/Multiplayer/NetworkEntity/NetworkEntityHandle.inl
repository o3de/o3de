/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace Multiplayer
{
    inline bool operator ==(const ConstNetworkEntityHandle& lhs, AZStd::nullptr_t)
    {
        return !lhs.Exists();
    }

    inline bool operator ==(AZStd::nullptr_t, const ConstNetworkEntityHandle& rhs)
    {
        return !rhs.Exists();
    }

    inline bool operator !=(const ConstNetworkEntityHandle& lhs, AZStd::nullptr_t)
    {
        return lhs.Exists();
    }

    inline bool operator !=(AZStd::nullptr_t, const ConstNetworkEntityHandle& rhs)
    {
        return rhs.Exists();
    }

    inline bool operator==(const ConstNetworkEntityHandle& lhs, const AZ::Entity* rhs)
    {
        return lhs.m_entity == rhs;
    }

    inline bool operator==(const AZ::Entity* lhs, const ConstNetworkEntityHandle& rhs)
    {
        return operator==(rhs, lhs);
    }

    inline bool operator!=(const ConstNetworkEntityHandle& lhs, const AZ::Entity* rhs)
    {
        return lhs.m_entity != rhs;
    }

    inline bool operator!=(const AZ::Entity* lhs, const ConstNetworkEntityHandle& rhs)
    {
        return operator!=(rhs, lhs);
    }

    inline NetEntityId ConstNetworkEntityHandle::GetNetEntityId() const
    {
        return m_netEntityId;
    }

    template <class ComponentType>
    inline const ComponentType* ConstNetworkEntityHandle::FindComponent() const
    {
        if (const AZ::Entity* entity{ GetEntity() })
        {
            return entity->template FindComponent<ComponentType>();
        }
        return nullptr;
    }

    inline bool ConstNetworkEntityHandle::Compare(const ConstNetworkEntityHandle& lhs, const ConstNetworkEntityHandle& rhs)
    {
        return lhs.m_netEntityId < rhs.m_netEntityId;
    }

    inline void NetworkEntityHandle::Init()
    {
        if (AZ::Entity* entity{ GetEntity() })
        {
            entity->Init();
        }
    }

    inline void NetworkEntityHandle::Activate()
    {
        if (AZ::Entity* entity{ GetEntity() })
        {
            entity->Activate();
        }
    }

    inline void NetworkEntityHandle::Deactivate()
    {
        if (AZ::Entity* entity{ GetEntity() })
        {
            entity->Deactivate();
        }
    }

    template <typename ControllerType>
    inline ControllerType* NetworkEntityHandle::FindController()
    {
        return static_cast<ControllerType*>(FindController(ControllerType::ComponentType::RTTI_Type()));
    }

    template <typename ComponentType>
    inline ComponentType* NetworkEntityHandle::FindComponent()
    {
        if (AZ::Entity* entity{ GetEntity() })
        {
            return entity->template FindComponent<ComponentType>();
        }
        return nullptr;
    }
}

//! AZStd::hash support.
namespace AZStd
{
    template <>
    class hash<Multiplayer::NetworkEntityHandle>
    {
    public:
        size_t operator()(const Multiplayer::NetworkEntityHandle &rhs) const
        {
            return hash<Multiplayer::NetEntityId>()(rhs.GetNetEntityId());
        }
    };

    template <>
    class hash<Multiplayer::ConstNetworkEntityHandle>
    {
    public:
        size_t operator()(const Multiplayer::ConstNetworkEntityHandle &rhs) const
        {
            return hash<Multiplayer::NetEntityId>()(rhs.GetNetEntityId());
        }
    };
}
