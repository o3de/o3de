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

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/hash.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace GraphCanvas
{
    struct Endpoint
    {
    public:
        AZ_CLASS_ALLOCATOR(Endpoint, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Endpoint, "{4AF80E61-8E0A-43F3-A560-769C925A113B}");

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

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Endpoint>()
                    ->Version(1)
                    ->Field("nodeId", &Endpoint::m_nodeId)
                    ->Field("slotId", &Endpoint::m_slotId)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Endpoint>("Endpoint", "Endpoint")
                        ->DataElement(0, &Endpoint::m_nodeId, "Node Id", "Node Id portion of endpoint")
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::DontGatherReference | AZ::Edit::SliceFlags::NotPushable)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ;
                }
            }
        }

        bool IsValid() const { return m_nodeId.IsValid() && m_slotId.IsValid(); }

        const AZ::EntityId& GetNodeId() const { return m_nodeId; }
        const AZ::EntityId& GetSlotId() const { return m_slotId; }

        void Clear()
        {
            m_nodeId.SetInvalid();
            m_slotId.SetInvalid();
        }

        AZ::EntityId m_nodeId;
        AZ::EntityId m_slotId;
    };
}

namespace AZStd
{
    template<>
    struct hash<GraphCanvas::Endpoint>
    {
        using argument_type = GraphCanvas::Endpoint;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& ref) const
        {
            result_type seed = 0;
            hash_combine(seed, ref.GetNodeId());
            hash_combine(seed, ref.GetSlotId());
            return seed;
        }

    };
}
