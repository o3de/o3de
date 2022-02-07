/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>

#include <Feature.h>
#include <FeatureMatrix.h>
#include <FrameDatabase.h>

namespace EMotionFX::MotionMatching
{
    class KdTree
    {
    public:
        AZ_RTTI(KdTree, "{CDA707EC-4150-463B-8157-90D98351ACED}")
        AZ_CLASS_ALLOCATOR_DECL

        KdTree() = default;
        virtual ~KdTree();

        bool Init(const FrameDatabase& frameDatabase,
            const FeatureMatrix& featureMatrix,
            const AZStd::vector<Feature*>& features,
            size_t maxDepth=10,
            size_t minFramesPerLeaf=1000);

        //! Calculate the number of dimensions or values for the given feature set.
        //! Each feature might store one or multiple values inside the feature matrix and the number of
        //! values each feature holds varies with the feature type. This calculates the sum of the number of
        //! values of the given feature set.
        static size_t CalcNumDimensions(const AZStd::vector<Feature*>& features);

        void Clear();
        void PrintStats();

        size_t GetNumNodes() const;
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

        void BuildTreeNodes(const FrameDatabase& frameDatabase,
            const FeatureMatrix& featureMatrix,
            const AZStd::vector<Feature*>& features,
            Node* node,
            Node* parent,
            size_t dimension = 0,
            bool leftSide = true);
        void FillFeatureValues(const FeatureMatrix& featureMatrix, const Feature* feature, size_t frameIndex, size_t startIndex);
        void FillFeatureValues(const FeatureMatrix& featureMatrix, const AZStd::vector<Feature*>& features, size_t frameIndex);
        void FillFramesForNode(Node* node,
            const FrameDatabase& frameDatabase,
            const FeatureMatrix& featureMatrix,
            const AZStd::vector<Feature*>& features,
            Node* parent,
            bool leftSide);
        void RecursiveCalcNumFrames(Node* node, size_t& outNumFrames) const;
        void ClearFramesForNonEssentialNodes();
        void MergeSmallLeafNodesToParents();
        void RemoveZeroFrameLeafNodes();
        void RemoveLeafNode(Node* node);
        void FindNearestNeighbors(Node* node, const AZStd::vector<float>& frameFloats, AZStd::vector<size_t>& resultFrameIndices) const;

    private:
        AZStd::vector<Node*> m_nodes;
        AZStd::vector<float> m_featureValues;
        size_t m_numDimensions = 0;
        size_t m_maxDepth = 20;
        size_t m_minFramesPerLeaf = 1000;
    };
} // namespace EMotionFX::MotionMatching
