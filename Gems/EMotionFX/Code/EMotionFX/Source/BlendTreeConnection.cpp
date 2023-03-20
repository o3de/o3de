/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeConnection.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeConnection, AnimGraphAllocator)

    BlendTreeConnection::BlendTreeConnection()
        : m_animGraph(nullptr)
        , m_sourceNode(nullptr)
        , m_id(AnimGraphConnectionId::Create())
        , m_sourcePort(MCORE_INVALIDINDEX16)
        , m_targetPort(MCORE_INVALIDINDEX16)
        , m_visited(false)
    {
    }

    BlendTreeConnection::BlendTreeConnection(AnimGraphNode* sourceNode, AZ::u16 sourcePort, AZ::u16 targetPort)
        : BlendTreeConnection()
    {
        if (sourceNode)
        {
            m_animGraph = sourceNode->GetAnimGraph();
        }

        m_sourcePort = sourcePort;
        m_targetPort = targetPort;

        SetSourceNode(sourceNode);
    }

    BlendTreeConnection::~BlendTreeConnection()
    {
    }

    void BlendTreeConnection::Reinit()
    {
        if (!m_animGraph)
        {
            return;
        }

        m_sourceNode = m_animGraph->RecursiveFindNodeById(GetSourceNodeId());
        AZ_Error("EMotionFX", m_sourceNode, "Could not find node for id %s.", GetSourceNodeId().ToString().c_str());
    }

    bool BlendTreeConnection::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!animGraph)
        {
            return false;
        }

        m_animGraph = animGraph;
        Reinit();
        return true;
    }

    void BlendTreeConnection::SetSourceNode(AnimGraphNode* node)
    {
        m_sourceNode = node;

        if (m_sourceNode)
        {
            m_sourceNodeId = m_sourceNode->GetId();
        }
    }

    bool BlendTreeConnection::GetIsValid() const
    {
        // make sure the node and input numbers are valid
        if (!m_sourceNode || m_sourcePort == MCORE_INVALIDINDEX16 || m_targetPort == MCORE_INVALIDINDEX16)
        {
            return false;
        }

        // the connection is valid
        return true;
    }

    void BlendTreeConnection::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeConnection>()
            ->Version(2)
            ->Field("id", &BlendTreeConnection::m_id)
            ->Field("sourceNodeId", &BlendTreeConnection::m_sourceNodeId)
            ->Field("sourcePortNr", &BlendTreeConnection::m_sourcePort)
            ->Field("targetPortNr", &BlendTreeConnection::m_targetPort);
    }
} // namespace EMotionFX
