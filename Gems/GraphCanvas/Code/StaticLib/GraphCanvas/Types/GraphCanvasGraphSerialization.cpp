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
#include <AzCore/std/any.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>

#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>

namespace GraphCanvas
{
    ///////////////////////
    // GraphSerialization
    ///////////////////////
    void GraphSerialization::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<GraphSerialization>()
            ->Version(2)
            ->Field("UserData", &GraphSerialization::m_userFields)
            ->Field("SceneData", &GraphSerialization::m_graphData)
            ->Field("Key", &GraphSerialization::m_serializationKey)
            ->Field("AveragePosition", &GraphSerialization::m_averagePosition)
            ->Field("ConnectedEndpoints", &GraphSerialization::m_connectedEndpoints)
        ;
    }

    GraphSerialization::GraphSerialization(AZStd::string serializationKey)
        : m_serializationKey(serializationKey)
    {
    }

    GraphSerialization::GraphSerialization(const QByteArray& dataArray)
    {
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        AZ::Utils::LoadObjectFromBufferInPlace(dataArray.constData(), (AZStd::size_t)dataArray.size(), *this, serializeContext);
        RegenerateIds();
    }

    GraphSerialization::GraphSerialization(GraphSerialization&& other)
        : m_serializationKey(other.m_serializationKey)
        , m_graphData(other.m_graphData)
        , m_averagePosition(other.m_averagePosition)
        , m_userFields(AZStd::move(other.m_userFields))
        , m_newIdMapping(AZStd::move(other.m_newIdMapping))
    {
    }

    GraphSerialization& GraphSerialization::operator=(GraphSerialization&& other)
    {
        m_serializationKey = other.m_serializationKey;
        m_graphData = other.m_graphData;
        m_averagePosition = other.m_averagePosition;
        m_userFields = AZStd::move(other.m_userFields);
        m_newIdMapping = AZStd::move(other.m_newIdMapping);
        return *this;
    }

    void GraphSerialization::Clear()
    {
        m_graphData.Clear();
        m_userFields.clear();
    }

    const AZStd::string& GraphSerialization::GetSerializationKey() const
    {
        return m_serializationKey;
    }

    void GraphSerialization::SetAveragePosition(const AZ::Vector2& averagePosition)
    {
        m_averagePosition = averagePosition;
    }

    const AZ::Vector2& GraphSerialization::GetAveragePosition() const
    {
        return m_averagePosition;
    }

    GraphData& GraphSerialization::GetGraphData()
    {
        return m_graphData;
    }

    const GraphData& GraphSerialization::GetGraphData() const
    {
        return m_graphData;
    }

    const AZStd::unordered_multimap< Endpoint, Endpoint >& GraphSerialization::GetConnectedEndpoints() const
    {
        return m_connectedEndpoints;
    }

    void GraphSerialization::SetConnectedEndpoints(const AZStd::unordered_multimap< Endpoint, Endpoint >& endpoints)
    {
        m_connectedEndpoints = endpoints;
    }

    AZStd::unordered_map<AZStd::string, AZStd::any>& GraphSerialization::GetUserDataMapRef()
    {
        return m_userFields;
    }

    const AZStd::unordered_map<AZStd::string, AZStd::any>& GraphSerialization::GetUserDataMapRef() const
    {
        return m_userFields;
    }

    AZ::EntityId GraphSerialization::FindRemappedEntityId(const AZ::EntityId& originalId) const
    {
        AZ::EntityId mappedId;

        auto mappingIter = m_newIdMapping.find(originalId);

        if (mappingIter != m_newIdMapping.end())
        {
            mappedId = mappingIter->second;
        }

        return mappedId;
    }

    void GraphSerialization::RegenerateIds()
    {
        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(this, m_newIdMapping);
    }
}
