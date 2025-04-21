/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphCanvas
{
    struct Endpoint
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_TYPE_INFO_WITH_NAME_DECL(Endpoint);

        Endpoint() = default;
        Endpoint(const AZ::EntityId& nodeId, const AZ::EntityId& slotId)
            : m_nodeId(nodeId)
            , m_slotId(slotId)
        {}

        ~Endpoint() = default;

        bool operator==(const Endpoint& other) const
        {
            return m_nodeId == other.m_nodeId && m_slotId == other.m_slotId;
        }

        bool operator!=(const Endpoint& other) const
        {
            return !(this->operator==(other));
        }

        static void Reflect(AZ::ReflectContext* reflection);
        bool IsValid() const { return m_nodeId.IsValid() && m_slotId.IsValid(); }

        constexpr const AZ::EntityId& GetNodeId() const { return m_nodeId; }
        constexpr const AZ::EntityId& GetSlotId() const { return m_slotId; }

        constexpr operator size_t() const
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, GetNodeId());
            AZStd::hash_combine(seed, GetSlotId());
            return seed;
        }

        void Clear()
        {
            m_nodeId.SetInvalid();
            m_slotId.SetInvalid();
        }

        AZ::EntityId m_nodeId;
        AZ::EntityId m_slotId;
    };
}
