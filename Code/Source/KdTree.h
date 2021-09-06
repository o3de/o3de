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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <FrameDatabase.h>
#include <FeatureDatabase.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        class KdTree
        {
        public:
            AZ_RTTI(KdTree, "{CDA707EC-4150-463B-8157-90D98351ACED}")
            AZ_CLASS_ALLOCATOR_DECL

            KdTree() = default;
            ~KdTree();

            bool Init(const FrameDatabase& frameDatabase, const FeatureDatabase& featureDatabase, size_t maxDepth=10, size_t minFramesPerLeaf=1000);
            void Clear();
            void PrintStats();

            size_t GetNumDimensions() const;
            size_t CalcMemoryUsageInBytes() const;
            bool IsInitialized() const;

            void FindNearestNeighbors(const AZStd::vector<float>& frameFloats, AZStd::vector<size_t>& resultFrameIndices) const;

        private:
            struct Node
            {
                Node* m_leftNode = nullptr;
                Node* m_rightNode = nullptr;
                Node* m_parent = nullptr;
                float m_median = 0.0f;
                size_t m_dimension = 0;
                AZStd::vector<size_t> m_frames;
            };

            void BuildTreeNodes(const FrameDatabase& frameDatabase, const FeatureDatabase& featureDatabase, Node* node, Node* parent, size_t dimension=0, bool leftSide=true);
            void FillFrameFloats(const FeatureDatabase& featureDatabase, size_t frameIndex);
            void FillFramesForNode(Node* node, const FrameDatabase& frameDatabase, const FeatureDatabase& featureDatabase, Node* parent, bool leftSide);
            void RecursiveCalcNumFrames(Node* node, size_t& outNumFrames) const;
            void ClearFramesForNonEssentialNodes();
            void MergeSmallLeafNodesToParents();
            void RemoveZeroFrameLeafNodes();
            void RemoveLeafNode(Node* node);
            void FindNearestNeighbors(Node* node, const AZStd::vector<float>& frameFloats, AZStd::vector<size_t>& resultFrameIndices) const;

        private:
            AZStd::vector<Node*> m_nodes;
            AZStd::vector<float> m_frameFloats;
            size_t m_numDimensions = 0;
            size_t m_maxDepth = 20;
            size_t m_minFramesPerLeaf = 1000;
        };
    } // namespace MotionMatching
} // namespace EMotionFX
