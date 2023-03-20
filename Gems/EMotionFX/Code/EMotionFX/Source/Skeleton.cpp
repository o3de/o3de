/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Skeleton.h"
#include "Node.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Skeleton, SkeletonAllocator)


    // constructor
    Skeleton::Skeleton()
    {
    }


    // destructor
    Skeleton::~Skeleton()
    {
        RemoveAllNodes();
    }


    // create the skeleton
    Skeleton* Skeleton::Create()
    {
        return aznew Skeleton();
    }


    // clone the skeleton
    Skeleton* Skeleton::Clone()
    {
        Skeleton* result = Skeleton::Create();

        result->ReserveNodes(m_nodes.size());
        result->m_rootNodes = m_rootNodes;

        // clone the nodes
        for (const Node* node : m_nodes)
        {
            result->AddNode(node->Clone(result));
        }

        result->m_bindPose = m_bindPose;

        return result;
    }


    // reserve memory
    void Skeleton::ReserveNodes(size_t numNodes)
    {
        m_nodes.reserve(numNodes);
    }


    // add a node
    void Skeleton::AddNode(Node* node)
    {
        m_nodes.emplace_back(node);
        m_nodesMap[node->GetNameString()] = node;
    }


    // remove a node
    void Skeleton::RemoveNode(size_t nodeIndex, bool delFromMem)
    {
        m_nodesMap.erase(m_nodes[nodeIndex]->GetNameString());
        if (delFromMem)
        {
            m_nodes[nodeIndex]->Destroy();
        }

        m_nodes.erase(AZStd::next(begin(m_nodes), nodeIndex));
    }


    // remove all nodes
    void Skeleton::RemoveAllNodes(bool delFromMem)
    {
        if (delFromMem)
        {
            for (Node* node : m_nodes)
            {
                node->Destroy();
            }
        }

        m_nodes.clear();
        m_nodesMap.clear();
        m_bindPose.Clear();
    }


    Node* Skeleton::FindNodeByName(const char* name) const
    {
        auto iter = m_nodesMap.find(name);
        if (iter != m_nodesMap.end())
        {
            return iter->second;
        }
        return nullptr;
    }


    Node* Skeleton::FindNodeByName(const AZStd::string& name) const
    {
        auto iter = m_nodesMap.find(name);
        if (iter != m_nodesMap.end())
        {
            return iter->second;
        }
        return nullptr;
    }


    // search on name, non case sensitive
    Node* Skeleton::FindNodeByNameNoCase(const char* name) const
    {
        // check the names for all nodes
        const auto foundNode = AZStd::find_if(begin(m_nodes), end(m_nodes), [name](const Node* node)
        {
            return AzFramework::StringFunc::Equal(node->GetNameString(), name, false /* no case */);
        });
        return foundNode != end(m_nodes) ? *foundNode : nullptr;
    }


    // search for a node on ID
    Node* Skeleton::FindNodeByID(size_t id) const
    {
        // check the ID's for all nodes
        const auto foundNode = AZStd::find_if(begin(m_nodes), end(m_nodes), [id](const Node* node)
        {
            return node->GetID() == id;
        });
        return foundNode != end(m_nodes) ? *foundNode : nullptr;
    }


    // set a given node
    void Skeleton::SetNode(size_t index, Node* node)
    {
        if (m_nodes[index])
        {
            m_nodesMap.erase(m_nodes[index]->GetNameString());
            m_nodes[index]->Destroy();
        }
        m_nodes[index] = node;
        m_nodesMap[node->GetNameString()] = node;
    }


    // set the number of nodes
    void Skeleton::SetNumNodes(size_t numNodes)
    {
        size_t oldLength = m_nodes.size();
        m_nodes.resize(numNodes);
        for (size_t i = oldLength; i < numNodes; ++i)
        {
            m_nodes[i] = nullptr;
        }
        m_bindPose.SetNumTransforms(numNodes);
    }


    // update the node indices
    void Skeleton::UpdateNodeIndexValues(size_t startNode)
    {
        const size_t numNodes = m_nodes.size();
        for (size_t i = startNode; i < numNodes; ++i)
        {
            m_nodes[i]->SetNodeIndex(i);
        }
    }


    // reserve memory for the root nodes array
    void Skeleton::ReserveRootNodes(size_t numNodes)
    {
        m_rootNodes.reserve(numNodes);
    }


    // add a root node
    void Skeleton::AddRootNode(size_t nodeIndex)
    {
        m_rootNodes.emplace_back(nodeIndex);
    }


    // remove a given root node
    void Skeleton::RemoveRootNode(size_t nr)
    {
        m_rootNodes.erase(AZStd::next(begin(m_rootNodes), nr));
    }


    // remove all root nodes
    void Skeleton::RemoveAllRootNodes()
    {
        m_rootNodes.clear();
    }


    // log all node names
    void Skeleton::LogNodes()
    {
        const size_t numNodes = m_nodes.size();
        for (size_t i = 0; i < numNodes; ++i)
        {
            MCore::LogInfo("%d = '%s'", i, m_nodes[i]->GetName());
        }
    }


    // calculate the hierarchy depth for a given node
    size_t Skeleton::CalcHierarchyDepthForNode(size_t nodeIndex) const
    {
        size_t result = 0;
        const Node* curNode = m_nodes[nodeIndex];
        while (curNode->GetParentNode())
        {
            result++;
            curNode = curNode->GetParentNode();
        }

        return result;
    }


    Node* Skeleton::FindNodeAndIndexByName(const AZStd::string& name, size_t& outIndex) const
    {
        if (name.empty())
        {
            outIndex = InvalidIndex;
            return nullptr;
        }

        Node* joint = FindNodeByNameNoCase(name.c_str());
        if (!joint)
        {
            outIndex = InvalidIndex;
            return nullptr;
        }

        outIndex = joint->GetNodeIndex();
        return joint;
    }
}   // namespace EMotionFX
