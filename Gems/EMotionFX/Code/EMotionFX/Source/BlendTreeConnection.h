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

        MCORE_INLINE AZ::u16 GetSourcePort() const                  { return mSourcePort; }
        MCORE_INLINE AZ::u16 GetTargetPort() const                  { return mTargetPort; }

        MCORE_INLINE void SetSourcePort(AZ::u16 sourcePort)         { mSourcePort = sourcePort; }
        MCORE_INLINE void SetTargetPort(AZ::u16 targetPort)         { mTargetPort = targetPort; }

        AnimGraphConnectionId GetId() const                         { return m_id; }
        void SetId(AnimGraphConnectionId id)                        { m_id = id; }

        MCORE_INLINE void SetIsVisited(bool visited)                { mVisited = visited; }
        MCORE_INLINE bool GetIsVisited() const                      { return mVisited; }

        AnimGraph* GetAnimGraph() const                             { return m_animGraph; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AnimGraph*              m_animGraph;
        AnimGraphNode*          m_sourceNode;       /**< The source node from which the incoming connection comes. */
        AZ::u64                 m_sourceNodeId;
        AZ::u64                 m_id;
        AZ::u16                 mSourcePort;        /**< The source port number, so the output port number of the node where the connection comes from. */
        AZ::u16                 mTargetPort;        /**< The target port number, which is the input port number of the target node. */
        bool                    mVisited;               /**< True when during updates this connection was used. */
    };
} // namespace EMotionFX