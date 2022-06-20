/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphNode;
    class AnimGraph;

    /**
     * A connection between two nodes.
     */
    class EMFX_API BlendTreeConnection
    {
    public:
        AZ_RTTI(BlendTreeConnection, "{B48FFEDB-87FB-4085-AE54-0302AC49373A}");
        AZ_CLASS_ALLOCATOR_DECL

        BlendTreeConnection();
        BlendTreeConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);
        virtual ~BlendTreeConnection();

        void Reinit();
        bool InitAfterLoading(AnimGraph* animGraph);

        bool GetIsValid() const;

        void SetSourceNode(AnimGraphNode* node);
        AZ_FORCE_INLINE AnimGraphNode* GetSourceNode() const        { return m_sourceNode; }
        AZ_FORCE_INLINE AnimGraphNodeId GetSourceNodeId() const     { return m_sourceNodeId; }

        MCORE_INLINE AZ::u16 GetSourcePort() const                  { return m_sourcePort; }
        MCORE_INLINE AZ::u16 GetTargetPort() const                  { return m_targetPort; }

        MCORE_INLINE void SetSourcePort(AZ::u16 sourcePort)         { m_sourcePort = sourcePort; }
        MCORE_INLINE void SetTargetPort(AZ::u16 targetPort)         { m_targetPort = targetPort; }

        AnimGraphConnectionId GetId() const                         { return m_id; }
        void SetId(AnimGraphConnectionId id)                        { m_id = id; }

        MCORE_INLINE void SetIsVisited(bool visited)                { m_visited = visited; }
        MCORE_INLINE bool GetIsVisited() const                      { return m_visited; }

        AnimGraph* GetAnimGraph() const                             { return m_animGraph; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AnimGraph*              m_animGraph;
        AnimGraphNode*          m_sourceNode;       /**< The source node from which the incoming connection comes. */
        AZ::u64                 m_sourceNodeId;
        AZ::u64                 m_id;
        AZ::u16                 m_sourcePort;        /**< The source port number, so the output port number of the node where the connection comes from. */
        AZ::u16                 m_targetPort;        /**< The target port number, which is the input port number of the target node. */
        bool                    m_visited;               /**< True when during updates this connection was used. */
    };
} // namespace EMotionFX
