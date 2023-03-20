/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Core.h"

namespace ScriptCanvas
{
    class Endpoint
    {
    public:

        AZ_CLASS_ALLOCATOR(Endpoint, AZ::SystemAllocator);
        AZ_TYPE_INFO(Endpoint, "{91D4ADAC-56FE-4D82-B9AF-6975D21435C8}");

        Endpoint() = default;
        Endpoint(const AZ::EntityId& nodeId, const SlotId& slotId);
        ~Endpoint() = default;

        bool operator==(const Endpoint& other) const;

        static void Reflect(AZ::ReflectContext* reflection);

        const AZ::EntityId& GetNodeId() const { return m_nodeId; }
        const SlotId& GetSlotId() const { return m_slotId; }

        bool IsValid() const { return m_nodeId.IsValid() && m_slotId.IsValid(); }

    protected:
        AZ::EntityId m_nodeId;
        SlotId m_slotId;
    };

    class NamedEndpoint
        : public Endpoint
    {
    public:

        AZ_CLASS_ALLOCATOR(NamedEndpoint, AZ::SystemAllocator);
        AZ_TYPE_INFO(NamedEndpoint, "{E4FAB996-1958-4445-8C8B-367F582773F7}");

        static void Reflect(AZ::ReflectContext* reflection);

        AZStd::string m_nodeName;
        AZStd::string m_slotName;

        NamedEndpoint() = default;
        explicit NamedEndpoint(const Endpoint& endpoint);
        NamedEndpoint
            ( const AZ::EntityId& nodeId
            , const AZStd::string& nodeName
            , const SlotId& slotId
            , const AZStd::string& slotName);

        ~NamedEndpoint() = default;

        const AZStd::string& GetNodeName() const;
        NamedNodeId GetNamedNodeId() const;

        const AZStd::string& GetSlotName() const;
        NamedSlotId GetNamedSlotId() const;

        bool operator==(const Endpoint& other) const;
        bool operator==(const NamedEndpoint& other) const;
    };
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Endpoint>
    {
        using argument_type = ScriptCanvas::Endpoint;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& ref) const
        {
            result_type seed = 0;
            hash_combine(seed, ref.GetNodeId());
            hash_combine(seed, ref.GetSlotId());
            return seed;
        }
    };

    template<>
    struct hash<ScriptCanvas::NamedEndpoint>
    {
        using argument_type = ScriptCanvas::NamedEndpoint;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& ref) const
        {
            return hash<ScriptCanvas::Endpoint>()(ref);
        }
    };
}
