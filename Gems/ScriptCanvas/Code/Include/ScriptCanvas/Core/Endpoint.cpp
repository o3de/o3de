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

#include "Endpoint.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    Endpoint::Endpoint(const AZ::EntityId& nodeId, const SlotId& slotId)
        : m_nodeId(nodeId)
        , m_slotId(slotId)
    {}

    bool Endpoint::operator==(const Endpoint& other) const
    {
        return m_nodeId == other.m_nodeId && m_slotId == other.m_slotId;
    }

    void Endpoint::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Endpoint>()
                ->Version(1)
                ->Field("nodeId", &Endpoint::m_nodeId)
                ->Field("slotId", &Endpoint::m_slotId)
                ;
        }
    }
    
    NamedEndpoint::NamedEndpoint
        ( const AZ::EntityId& nodeId
        , const AZStd::string& nodeName
        , const SlotId& slotId
        , const AZStd::string& slotName)
        : Endpoint(nodeId, slotId)
        , m_nodeName(nodeName)
        , m_slotName(slotName)
    {}
    
    NamedEndpoint::NamedEndpoint(const Endpoint& endpoint)
        : Endpoint(endpoint)
    {}
    
    const AZStd::string& NamedEndpoint::GetNodeName() const
    {
        return m_nodeName;
    }

    NamedNodeId NamedEndpoint::GetNamedNodeId() const
    {
        return NamedNodeId(m_nodeId, m_nodeName);
    }

    const AZStd::string& NamedEndpoint::GetSlotName() const
    {
        return m_slotName;
    }

    NamedSlotId NamedEndpoint::GetNamedSlotId() const
    {
        return NamedSlotId(m_slotId, m_nodeName);
    }
    
    bool NamedEndpoint::operator==(const Endpoint& other) const
    {
        return Endpoint::operator==(other);
    }

    bool NamedEndpoint::operator==(const NamedEndpoint& other) const
    {
        return Endpoint::operator==(other);
    }

    void NamedEndpoint::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<NamedEndpoint, Endpoint>()
                ->Version(0)
                ->Field("nodeName", &NamedEndpoint::m_nodeName)
                ->Field("slotName", &NamedEndpoint::m_slotName)
                ;
        }

    }
}
