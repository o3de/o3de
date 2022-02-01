/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <KdTree.h>
#include <Feature.h>
#include <Allocators.h>

#include <AzCore/Debug/Timer.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(KdTree, MotionMatchAllocator, 0)

    KdTree::~KdTree()
    {
        Clear();
    }

    size_t KdTree::CalcNumDimensions(const AZStd::vector<Feature*>& features)
    {
        size_t result = 0;
        for (Feature* feature : features)
        {
            if (feature->GetId().IsNull())
            {
                continue;
            }

            result += feature->GetNumDimensions();
        }
        return result;
    }

    bool KdTree::Init(const FrameDatabase& frameDatabase,
        const FeatureMatrix& featureMatrix,
        const AZStd::vector<Feature*>& features,
        size_t maxDepth,
        size_t minFramesPerLeaf)
    {
        AZ::Debug::Timer timer;
        timer.Stamp();

        Clear();

        // Verify the dimensions.
        m_numDimensions = CalcNumDimensions(features);

        // Going above a 48 dimensional tree would start eating up too much memory.
        const size_t maxNumDimensions = 48;
        if (m_numDimensions == 0 || m_numDimensions > maxNumDimensions)
        {
            AZ_Error("Motion Matching", false, "Cannot initialize KD-tree. KD-tree dimension (%d) has to be between 1 and %zu. Please use Feature::SetIncludeInKdTree(false) on some features.", m_numDimensions, maxNumDimensions);
            return false;
        }

        if (minFramesPerLeaf > 100000)
        {
            AZ_Error("Motion Matching", false, "KdTree minFramesPerLeaf (%d) cannot be smaller than 100000.", minFramesPerLeaf);
            return false;
        }

        if (maxDepth == 0)
        {
            AZ_Error("Motion Matching", false, "KdTree max depth (%d) cannot be zero", maxDepth);
            return false;
        }

        m_maxDepth = maxDepth;
        m_minFramesPerLeaf = minFramesPerLeaf;

        // Build the tree.
        m_featureValues.resize(m_numDimensions);
        BuildTreeNodes(frameDatabase, featureMatrix, features, new Node(), nullptr, 0);
        MergeSmallLeafNodesToParents();
        ClearFramesForNonEssentialNodes();
        RemoveZeroFrameLeafNodes();

        const float initTime = timer.GetDeltaTimeInSeconds();
        AZ_TracePrintf("EMotionFX", "KdTree initialized in %f seconds (numNodes = %d  numDims = %d  Memory used = %.2f MB).",
            initTime, m_nodes.size(),
            m_numDimensions,
            static_cast<float>(CalcMemoryUsageInBytes()) / 1024.0f / 1024.0f);

        PrintStats();
        return true;
    }

    void KdTree::Clear()
    {
        // delete all nodes
        for (Node* node : m_nodes)
        {
            delete node;
        }

        m_nodes.clear();
        m_featureValues.clear();
        m_numDimensions = 0;
    }

    size_t KdTree::CalcMemoryUsageInBytes() const
    {
        size_t totalBytes = 0;

        for (const Node* node : m_nodes)
        {
            totalBytes += sizeof(Node);
            totalBytes += node->m_frames.capacity() * sizeof(size_t);
        }

        totalBytes += m_featureValues.capacity() * sizeof(float);
        totalBytes += sizeof(KdTree);
        return totalBytes;
    }

    bool KdTree::IsInitialized() const
    {
        return (m_numDimensions != 0);
    }

    size_t KdTree::GetNumNodes() const
    {
        return m_nodes.size();
    }

    size_t KdTree::GetNumDimensions() const
    {
        return m_numDimensions;
    }

    void KdTree::BuildTreeNodes(const FrameDatabase& frameDatabase,
        const FeatureMatrix& featureMatrix,
        const AZStd::vector<Feature*>& features,
        Node* node,
        Node* parent,
        size_t dimension,
        bool leftSide)
    {
        node->m_parent = parent;
        node->m_dimension = dimension;
        m_nodes.emplace_back(node);

        // Fill the frames array and calculate the median.
        FillFramesForNode(node, frameDatabase, featureMatrix, features, parent, leftSide);

        // Prevent splitting further when we don't want to.
        const size_t maxDimensions = AZ::GetMin(m_numDimensions, m_maxDepth);
        if (node->m_frames.size() < m_minFramesPerLeaf * 2 ||
            dimension >= maxDimensions)
        {
            return;
        }

        // Create the left node.
        Node* leftNode = new Node();
        AZ_Assert(!node->m_leftNode, "Expected the parent left node to be a nullptr");
        node->m_leftNode = leftNode;
        BuildTreeNodes(frameDatabase, featureMatrix, features, leftNode, node, dimension + 1, true);

        // Create the right node.
        Node* rightNode = new Node();
        AZ_Assert(!node->m_rightNode, "Expected the parent right node to be a nullptr");
        node->m_rightNode = rightNode;
        BuildTreeNodes(frameDatabase, featureMatrix, features, rightNode, node, dimension + 1, false);
    }

    void KdTree::ClearFramesForNonEssentialNodes()
    {
        for (Node* node : m_nodes)
        {
            if (node->m_leftNode && node->m_rightNode)
            {
                node->m_frames.clear();
                node->m_frames.shrink_to_fit();
            }
        }
    }

    void KdTree::RemoveLeafNode(Node* node)
    {
        Node* parent = node->m_parent;

        if (parent->m_leftNode == node)
        {
            parent->m_leftNode = nullptr;
        }

        if (parent->m_rightNode == node)
        {
            parent->m_rightNode = nullptr;
        }

        // Remove it from the node vector.
        const auto location = AZStd::find(m_nodes.begin(), m_nodes.end(), node);
        AZ_Assert(location != m_nodes.end(), "Expected to find the item to remove.");
        m_nodes.erase(location);

        delete node;
    }

    void KdTree::MergeSmallLeafNodesToParents()
    {
        // If the tree is empty or only has a single node, there is nothing to merge.
        if (m_nodes.size() < 2)
        {
            return;
        }

        AZStd::vector<Node*> nodesToRemove;
        for (Node* node : m_nodes)
        {
            // If we are a leaf node and we don't have enough frames.
            if ((!node->m_leftNode && !node->m_rightNode) &&
                node->m_frames.size() < m_minFramesPerLeaf)
            {
                nodesToRemove.emplace_back(node);
            }
        }

        // Remove the actual nodes.
        for (Node* node : nodesToRemove)
        {
            RemoveLeafNode(node);
        }
    }

    void KdTree::RemoveZeroFrameLeafNodes()
    {
        AZStd::vector<Node*> nodesToRemove;

        // Build a list of leaf nodes to remove.
        // These are ones that have no feature inside them.
        for (Node* node : m_nodes)
        {
            if ((!node->m_leftNode && !node->m_rightNode) &&
                node->m_frames.empty())
            {
                nodesToRemove.emplace_back(node);
            }
        }

        // Remove the actual nodes.
        for (Node* node : nodesToRemove)
        {
            RemoveLeafNode(node);
        }
    }

    void KdTree::FillFramesForNode(Node* node,
        const FrameDatabase& frameDatabase,
        const FeatureMatrix& featureMatrix,
        const AZStd::vector<Feature*>& features,
        Node* parent,
        bool leftSide)
    {
        float median = 0.0f;
        if (parent)
        {
            // Assume half of the parent frames are in this node.
            node->m_frames.reserve((parent->m_frames.size() / 2) + 1);

            // Add parent frames to this node, but only ones that should be on this side.
            for (const size_t frameIndex : parent->m_frames)
            {
                FillFeatureValues(featureMatrix, features, frameIndex);

                const float value = m_featureValues[parent->m_dimension];
                if (leftSide)
                {
                    if (value <= parent->m_median)
                    {
                        node->m_frames.emplace_back(frameIndex);
                    }
                }
                else
                {
                    if (value > parent->m_median)
                    {
                        node->m_frames.emplace_back(frameIndex);
                    }
                }

                median += value;
            }
        }
        else // We're the root node.
        {
            node->m_frames.reserve(frameDatabase.GetNumFrames());
            for (const Frame& frame : frameDatabase.GetFrames())
            {
                const size_t frameIndex = frame.GetFrameIndex();
                node->m_frames.emplace_back(frameIndex);
                FillFeatureValues(featureMatrix, features, frameIndex);
                median += m_featureValues[node->m_dimension];
            }
        }

        if (!node->m_frames.empty())
        {
            median /= static_cast<float>(node->m_frames.size());
        }
        node->m_median = median;
    }

    void KdTree::FillFeatureValues(const FeatureMatrix& featureMatrix, const Feature* feature, size_t frameIndex, size_t startIndex)
    {
        const size_t numDimensions = feature->GetNumDimensions();
        const size_t featureColumnOffset = feature->GetColumnOffset();
        for (size_t i = 0; i < numDimensions; ++i)
        {
            m_featureValues[startIndex + i] = featureMatrix(frameIndex, featureColumnOffset + i);
        }
    }

    void KdTree::FillFeatureValues(const FeatureMatrix& featureMatrix, const AZStd::vector<Feature*>& features, size_t frameIndex)
    {
        size_t startDimension = 0;
        for (const Feature* feature : features)
        {
            FillFeatureValues(featureMatrix, feature, frameIndex, startDimension);
            startDimension += feature->GetNumDimensions();
        }
    }

    void KdTree::RecursiveCalcNumFrames(Node* node, size_t& outNumFrames) const
    {
        if (node->m_leftNode && node->m_rightNode)
        {
            RecursiveCalcNumFrames(node->m_leftNode, outNumFrames);
            RecursiveCalcNumFrames(node->m_rightNode, outNumFrames);
        }
        else
        {
            outNumFrames += node->m_frames.size();
        }
    }

    void KdTree::PrintStats()
    {
        size_t leftNumFrames = 0;
        size_t rightNumFrames = 0;
        if (m_nodes[0]->m_leftNode)
        {
            RecursiveCalcNumFrames(m_nodes[0]->m_leftNode, leftNumFrames);
        }

        if (m_nodes[0]->m_rightNode)
        {
            RecursiveCalcNumFrames(m_nodes[0]->m_rightNode, rightNumFrames);
        }

        const float numFrames = static_cast<float>(leftNumFrames + rightNumFrames);
        const float halfFrames = numFrames / 2.0f;
        const float balanceScore = 100.0f - (AZ::GetAbs(halfFrames - static_cast<float>(leftNumFrames)) / numFrames) * 100.0f;

        // Get the maximum depth.
        size_t maxDepth = 0;
        for (const Node* node : m_nodes)
        {
            maxDepth = AZ::GetMax(maxDepth, node->m_dimension);
        }

        AZ_TracePrintf("EMotionFX", "KdTree Balance Info: leftSide=%d rightSide=%d score=%.2f totalFrames=%d maxDepth=%d", leftNumFrames, rightNumFrames, balanceScore, leftNumFrames + rightNumFrames, maxDepth);

        size_t numLeafNodes = 0;
        size_t numZeroNodes = 0;
        size_t minFrames = 1000000000;
        size_t maxFrames = 0;
        for (const Node* node : m_nodes)
        {
            if (node->m_leftNode || node->m_rightNode)
            {
                continue;
            }

            numLeafNodes++;

            if (node->m_frames.empty())
            {
                numZeroNodes++;
            }

            AZ_TracePrintf("EMotionFX", "Frames = %d", node->m_frames.size());

            minFrames = AZ::GetMin(minFrames, node->m_frames.size());
            maxFrames = AZ::GetMax(maxFrames, node->m_frames.size());
        }

        const size_t avgFrames = (leftNumFrames + rightNumFrames) / numLeafNodes;
        AZ_TracePrintf("EMotionFX", "KdTree Node Info: leafs=%d avgFrames=%d zeroFrames=%d minFrames=%d maxFrames=%d", numLeafNodes, avgFrames, numZeroNodes, minFrames, maxFrames);
    }

    void KdTree::FindNearestNeighbors(const AZStd::vector<float>& frameFloats, AZStd::vector<size_t>& resultFrameIndices) const
    {
        AZ_Assert(IsInitialized() && !m_nodes.empty(), "Expecting a valid and initialized kdTree. Did you forget to call KdTree::Init()?");
        Node* curNode = m_nodes[0];

        // Step as far as we need to through the kdTree.
        Node* nodeToSearch = nullptr;
        const size_t numDimensions = frameFloats.size();
        for (size_t d = 0; d < numDimensions; ++d)
        {
            AZ_Assert(curNode->m_dimension == d, "Dimension mismatch");

            // We have children in both directions.
            if (curNode->m_leftNode && curNode->m_rightNode)
            {
                curNode = (frameFloats[d] <= curNode->m_median) ? curNode->m_leftNode : curNode->m_rightNode;
            }
            else if (!curNode->m_leftNode && !curNode->m_rightNode) // we have a leaf node
            {
                nodeToSearch = curNode;
            }
            else
            {
                // We have either a left and right node, so we're not at a leaf yet.
                if (curNode->m_leftNode)
                {
                    if (frameFloats[d] <= curNode->m_median)
                    {
                        curNode = curNode->m_leftNode;
                    }
                    else
                    {
                        nodeToSearch = curNode;
                    }
                }
                else // We have a right node.
                {
                    if (frameFloats[d] > curNode->m_median)
                    {
                        curNode = curNode->m_rightNode;
                    }
                    else
                    {
                        nodeToSearch = curNode;
                    }
                }
            }

            // If we found our search node, perform a linear search through the frames inside this node.
            if (nodeToSearch)
            {
                //AZ_Assert(d == nodeToSearch->m_dimension, "Dimension mismatch inside kdTree nearest neighbor search.");
                FindNearestNeighbors(nodeToSearch, frameFloats, resultFrameIndices);
                return;
            }
        }

        FindNearestNeighbors(curNode, frameFloats, resultFrameIndices);
    }

    void KdTree::FindNearestNeighbors([[maybe_unused]] Node* node, [[maybe_unused]] const AZStd::vector<float>& frameFloats, AZStd::vector<size_t>& resultFrameIndices) const
    {
        resultFrameIndices = node->m_frames;
    }
} // namespace EMotionFX::MotionMatching
