/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/Debug/Timer.h>

#include <KdTree.h>
#include <Feature.h>
#include <Allocators.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(KdTree, MotionMatchAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(KdTree::Node, MotionMatchAllocator);

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
        AZ_PROFILE_SCOPE(Animation, "MotionMatchingData::InitKdTree");

#if !defined(_RELEASE)
        AZ::Debug::Timer timer;
        timer.Stamp();
#endif

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
            AZ_Error("Motion Matching", false, "KdTree minFramesPerLeaf (%d) cannot be bigger than 100000.", minFramesPerLeaf);
            return false;
        }

        if (maxDepth == 0)
        {
            AZ_Error("Motion Matching", false, "KdTree max depth (%d) cannot be zero", maxDepth);
            return false;
        }

        if (frameDatabase.GetNumFrames() == 0)
        {
            AZ_Error("Motion Matching", false, "Skipping to initialize KD-tree. No frames in the motion database.");
            return true;
        }

        m_maxDepth = maxDepth;
        m_minFramesPerLeaf = minFramesPerLeaf;

        // Not all features are present in the KD-tree, thus we need to remap KD-tree local feature columns to the
        // feature schema global feature columns.
        const AZStd::vector<size_t> localToSchemaFeatureColumns = CalcLocalToSchemaFeatureColumns(features);

        // Build the tree.
        BuildTreeNodes(frameDatabase, featureMatrix, localToSchemaFeatureColumns, aznew Node(), nullptr, 0);
        MergeSmallLeafNodesToParents();
        ClearFramesForNonEssentialNodes();
        RemoveZeroFrameLeafNodes();

#if !defined(_RELEASE)
        const float initTime = timer.GetDeltaTimeInSeconds();
        AZ_TracePrintf("Motion Matching", "KD-Tree initialized in %.2f ms (numNodes = %d  numDims = %d  Memory used = %.2f MB).",
            initTime * 1000.0f,
            m_nodes.size(),
            m_numDimensions,
            static_cast<float>(CalcMemoryUsageInBytes()) / 1024.0f / 1024.0f);

        PrintStats();
#endif
        return true;
    }

    AZStd::vector<size_t> KdTree::CalcLocalToSchemaFeatureColumns(const AZStd::vector<Feature*>& features) const
    {
        AZStd::vector<size_t> localToSchemaFeatureColumns;
        localToSchemaFeatureColumns.resize(m_numDimensions);

        size_t currentColumn = 0;
        for (const Feature* feature : features)
        {
            const size_t numDimensions = feature->GetNumDimensions();
            const size_t featureColumnOffset = feature->GetColumnOffset();
            for (size_t i = 0; i < numDimensions; ++i)
            {
                localToSchemaFeatureColumns[currentColumn] = featureColumnOffset + i;
                currentColumn++;
            }
        }

        AZ_Assert(m_numDimensions == currentColumn, "There should be a column index mapping for each of the available dimensions.");
        return localToSchemaFeatureColumns;
    }

    void KdTree::Clear()
    {
        // delete all nodes
        for (Node* node : m_nodes)
        {
            delete node;
        }

        m_nodes.clear();
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
        const AZStd::vector<size_t>& localToSchemaFeatureColumns,
        Node* node,
        Node* parent,
        size_t dimension,
        bool leftSide)
    {
        node->m_parent = parent;
        node->m_dimension = dimension;
        m_nodes.emplace_back(node);

        // Fill the frames array and calculate the median.
        AZStd::vector<float> frameFeatureValues;
        FillFramesForNode(node, frameDatabase, featureMatrix, localToSchemaFeatureColumns, frameFeatureValues, parent, leftSide);

        // Prevent splitting further when we don't want to.
        const size_t maxDimensions = AZ::GetMin(m_numDimensions, m_maxDepth);
        if (node->m_frames.size() < m_minFramesPerLeaf * 2 ||
            dimension >= maxDimensions)
        {
            return;
        }

        // Create the left node.
        Node* leftNode = aznew Node();
        AZ_Assert(!node->m_leftNode, "Expected the parent left node to be a nullptr");
        node->m_leftNode = leftNode;
        BuildTreeNodes(frameDatabase, featureMatrix, localToSchemaFeatureColumns, leftNode, node, dimension + 1, true);

        // Create the right node.
        Node* rightNode = aznew Node();
        AZ_Assert(!node->m_rightNode, "Expected the parent right node to be a nullptr");
        node->m_rightNode = rightNode;
        BuildTreeNodes(frameDatabase, featureMatrix, localToSchemaFeatureColumns, rightNode, node, dimension + 1, false);
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
        const AZStd::vector<size_t>& localToSchemaFeatureColumns,
        AZStd::vector<float>& frameFeatureValues,
        Node* parent,
        bool leftSide)
    {
        frameFeatureValues.clear();

        if (parent)
        {
            // Assume half of the parent frames are in this node.
            const size_t numExpectedFrames = (parent->m_frames.size() / 2) + 1;
            frameFeatureValues.reserve(numExpectedFrames);
            node->m_frames.reserve(numExpectedFrames);

            // Add parent frames to this node, but only ones that should be on this side.
            for (const size_t frameIndex : parent->m_frames)
            {
                // Remap local to the KD-tree feature column to the feature schema global column and read the value directly from the feature matrix.
                const float featureValue = featureMatrix(frameIndex, localToSchemaFeatureColumns[parent->m_dimension]);
                frameFeatureValues.push_back(featureValue);

                if (leftSide)
                {
                    if (featureValue <= parent->m_median)
                    {
                        node->m_frames.emplace_back(frameIndex);
                    }
                }
                else
                {
                    if (featureValue > parent->m_median)
                    {
                        node->m_frames.emplace_back(frameIndex);
                    }
                }
            }
        }
        else // We're the root node.
        {
            node->m_frames.reserve(frameDatabase.GetNumFrames());
            for (const Frame& frame : frameDatabase.GetFrames())
            {
                const size_t frameIndex = frame.GetFrameIndex();
                node->m_frames.emplace_back(frameIndex);

                // Remap local to the KD-tree feature column to the feature schema global column and read the value directly from the feature matrix.
                const float featureValue = featureMatrix(frameIndex, localToSchemaFeatureColumns[node->m_dimension]);
                frameFeatureValues.push_back(featureValue);
            }
        }

        // Calculate the median in O(n).
        node->m_median = 0.0f;
        if (!frameFeatureValues.empty())
        {
            auto medianIterator = frameFeatureValues.begin() + frameFeatureValues.size() / 2;
            AZStd::nth_element(frameFeatureValues.begin(), medianIterator, frameFeatureValues.end());
            node->m_median = frameFeatureValues[frameFeatureValues.size() / 2];
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
#if !defined(_RELEASE)
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

        AZ_TracePrintf("Motion Matching", "    KdTree Balance Info: leftSide=%d rightSide=%d score=%.2f totalFrames=%d maxDepth=%d", leftNumFrames, rightNumFrames, balanceScore, leftNumFrames + rightNumFrames, maxDepth);

        size_t numLeafNodes = 0;
        size_t numZeroNodes = 0;
        size_t minFrames = 1000000000;
        size_t maxFrames = 0;
        AZStd::string framesString;
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

            if (!framesString.empty())
            {
                framesString += ", ";
            }
            framesString += AZStd::to_string(node->m_frames.size());

            minFrames = AZ::GetMin(minFrames, node->m_frames.size());
            maxFrames = AZ::GetMax(maxFrames, node->m_frames.size());
        }
        AZ_TracePrintf("Motion Matching", "    Frames = (%s)", framesString.c_str());

        const size_t avgFrames = (leftNumFrames + rightNumFrames) / numLeafNodes;
        AZ_TracePrintf("Motion Matching", "    KdTree Node Info: leafs=%d avgFrames=%d zeroFrames=%d minFrames=%d maxFrames=%d", numLeafNodes, avgFrames, numZeroNodes, minFrames, maxFrames);
#endif
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
