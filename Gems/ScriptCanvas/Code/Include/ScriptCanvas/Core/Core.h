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
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/hash.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/NamedEntityId.h>

#include <Core/NamedId.h>

// #define EXPRESSION_TEMPLATES_ENABLED

namespace AZ
{
    class Entity;
    class ReflectContext;
}

namespace ScriptCanvas
{
    // A place holder identifier for the AZ::Entity that owns the graph.
    // The actual value in each location initialized to GraphOwnerId is populated with the owning entity at editor-time, Asset Processor-time, or runtime, as soon as the owning entity is known.
    using GraphOwnerIdType = AZ::EntityId;
    static const GraphOwnerIdType GraphOwnerId = AZ::EntityId(0xacedc0de);
    
    // A place holder identifier for unique runtime graph on Entity that is running more than one instance of the same graph.
    // This allows multiple instances of the same graph to be addressed individually on the same entity.
    // The actual value in each location initialized to UniqueId is populated at run-time.
    using RuntimeIdType = AZ::EntityId;
    static const RuntimeIdType UniqueId = AZ::EntityId(0xfee1baad);

    class Node;
    class Edge;

    using ID = AZ::EntityId;
    using NodeIdList = AZStd::vector<ID>;
    using NodePtrList = AZStd::vector<Node*>;
    using NodePtrConstList = AZStd::vector<const Node*>;
    
    enum class ExecutionMode : AZ::u8
    {
        GraphTraversal,
        Interpreted,
        Native,
    };

    struct SlotId
    {
        AZ_TYPE_INFO(SlotId, "{14C629F6-467B-46FE-8B63-48FDFCA42175}");

        AZ::Uuid m_id{ AZ::Uuid::CreateNull() };

        SlotId() = default;

        SlotId(const SlotId&) = default;

        explicit SlotId(const AZ::Uuid& uniqueId)
            : m_id(uniqueId)
        {}

        //! AZ::Uuid has a constructor not marked as explicit that accepts a const char*
        //! Adding a constructor which accepts a const char* and deleting it prevents
        //! AZ::Uuid from being initialized with c-strings
        explicit SlotId(const char* str) = delete;

        static void Reflect(AZ::ReflectContext* context);

        bool IsValid() const
        {
            return m_id != AZ::Uuid::CreateNull();
        }

        AZStd::string ToString() const
        {
            return m_id.ToString<AZStd::string>();
        }

        bool operator==(const SlotId& rhs) const
        {
            return m_id == rhs.m_id;
        }

        bool operator!=(const SlotId& rhs) const
        {
            return m_id != rhs.m_id;
        }

    };

    using NamedActiveEntityId = AZ::NamedEntityId;    
    using NamedNodeId = NamedId<AZ::EntityId>;
    using NamedSlotId = NamedId<SlotId>;

    using NodeTypeIdentifier = AZStd::size_t;
    using EBusEventId = AZ::Crc32;
    using EBusBusId = AZ::Crc32;
    using ScriptCanvasId = AZ::EntityId;
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::SlotId>
    {
        using argument_type = ScriptCanvas::SlotId;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& ref) const
        {
            return AZStd::hash<AZ::Uuid>()(ref.m_id);
        }

    };
}

#define SCRIPT_CANVAS_INFINITE_LOOP_DETECTION_COUNT (2000000)