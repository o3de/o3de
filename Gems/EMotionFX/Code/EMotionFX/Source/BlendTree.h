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

// include the required headers
#include "EMotionFXConfig.h"
#include "BlendTreeFinalNode.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Pose;
    class AnimGraphNode;


    /**
     *
     *
     *
     */
    class EMFX_API BlendTree
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTree, "{A8B5BB1E-5BA9-4B0A-88E9-21BB7A199ED2}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
        };

        BlendTree();
        ~BlendTree();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        // overloaded
        const char* GetPaletteName() const override                     { return "Blend Tree"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override  { return AnimGraphObject::CATEGORY_SOURCES; }
        bool GetCanActAsState() const override                          { return true; }
        bool GetHasVisualGraph() const override                         { return true; }
        bool GetCanHaveChildren() const override                        { return true; }
        bool GetSupportsDisable() const override                        { return true; }
        bool GetSupportsVisualization() const override                  { return true; }
        AZ::Color GetVisualColor() const override                       { return AZ::Color(0.21f, 0.67f, 0.21f, 1.0f); }
        AZ::Color GetHasChildIndicatorColor() const override            { return AZ::Color(0.0f, 0.76f, 0.27f, 1.0f); }

        void Rewind(AnimGraphInstance* animGraphInstance) override;
        bool GetHasOutputPose() const override                          { return true; }

        void SetVirtualFinalNode(AnimGraphNode* node);
        MCORE_INLINE AnimGraphNode* GetVirtualFinalNode() const         { return mVirtualFinalNode; }

        void SetFinalNodeId(const AnimGraphNodeId finalNodeId);
        AZ_FORCE_INLINE AnimGraphNodeId GetFinalNodeId() const          { return m_finalNodeId; }
        AZ_FORCE_INLINE BlendTreeFinalNode* GetFinalNode()              { return m_finalNode; }

        // remove the node and auto delete connections to this node
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled) override;
        AnimGraphNode* GetRealFinalNode() const;

        void GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const override;
        
        /**
        * Find cycles in this blend tree. 
        * @result  If cycles are not found, an empty vector is returned. If cycles are found, connections that break the cycles are returned. The pair being returned
        *          contains the connection and the target node (since BlendTreeConnection does not contain the target node)
        */
        AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>> FindCycles() const;

        /**
        * Indicates if creating a connection from sourceNode to target will produce a cycle.
        * param[in] sourceNode source node of the connection
        * param[in] targetNdoe target node of the connection
        * @result  true if creating the connection will produce a cycle. False otherwise
        */
        bool ConnectionWillProduceCycle(AnimGraphNode* sourceNode, AnimGraphNode* targetNode) const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::u64                 m_finalNodeId;      /**< Id of the final node that gets serialized. The final node represents the output of the blend tree. */
        BlendTreeFinalNode*     m_finalNode;        /**< The cached final node pointer based on the final node id. */
        AnimGraphNode*          mVirtualFinalNode;  /**< The virtual final node, which is the node who's output is used as final output. A value of nullptr means it will use the real mFinalNode. */

        /**
        * Helper function that recursively (through incoming connections) detect cycles. The function performs a DFS to find back edges (connections to itself or to one of its ancestors).
        * @param[in] nextNode the current node being analyzed
        * @param[in] visitedNodes nodes currently visited by the algorithm. This will mark all nodes that are currently in the DFS. 
        * @param[out] cycleConnections if a cycle was detected, the connection that produced the cycle is returned
        */
        void RecursiveFindCycles(AnimGraphNode* nextNode, AZStd::unordered_set<AnimGraphNode*>& visitedNodes, AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*>>& cycleConnections) const;

        void RecursiveSetUniqueDataFlag(AnimGraphNode* startNode, AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled);
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
