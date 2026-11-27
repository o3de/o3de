/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
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

        // Visual-only bend points in graph space, ordered from the source to the target port.
        // They are editor layout: evaluating the blend tree never reads them.
        const AZStd::vector<AZ::Vector2>& GetWaypoints() const      { return m_waypoints; }
        void SetWaypoints(const AZStd::vector<AZ::Vector2>& waypoints) { m_waypoints = waypoints; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AnimGraph*              m_animGraph;
        AnimGraphNode*          m_sourceNode;       /**< The source node from which the incoming connection comes. */
        AZ::u64                 m_sourceNodeId;
        AZ::u64                 m_id;
        AZ::u16                 m_sourcePort;        /**< The source port number, so the output port number of the node where the connection comes from. */
        AZ::u16                 m_targetPort;        /**< The target port number, which is the input port number of the target node. */
        bool                    m_visited;               /**< True when during updates this connection was used. */
        AZStd::vector<AZ::Vector2> m_waypoints;
    };

    // Editor layout: a wire from an output that ends at a pin, which more wires can branch from.
    // The blend tree never evaluates it, so it means the same as an unconnected output.
    class EMFX_API BlendTreePinnedWire
    {
    public:
        AZ_TYPE_INFO(BlendTreePinnedWire, "{14937EC9-A617-49BE-B1CE-50792010391C}");

        BlendTreePinnedWire() = default;
        BlendTreePinnedWire(AZ::u16 sourcePort, const AZStd::vector<AZ::Vector2>& waypoints)
            : m_sourcePort(sourcePort)
            , m_waypoints(waypoints)
        {
        }

        bool operator==(const BlendTreePinnedWire& other) const
        {
            return m_sourcePort == other.m_sourcePort && m_waypoints == other.m_waypoints;
        }

        static void Reflect(AZ::ReflectContext* context);

        AZ::u16 m_sourcePort = 0;
        // Bend points in graph space from the output onwards. The last one is the pin the wire ends at.
        AZStd::vector<AZ::Vector2> m_waypoints;
    };
} // namespace EMotionFX
