/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(Skeleton, SkeletonAllocator, 0)


    // constructor
    Skeleton::Skeleton()
    {
        m_nodes.SetMemoryCategory(EMFX_MEMCATEGORY_SKELETON);
        m_rootNodes.SetMemoryCategory(EMFX_MEMCATEGORY_SKELETON);
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

        const uint32 numNodes = m_nodes.GetLength();
        result->ReserveNodes(numNodes);
        result->m_rootNodes = m_rootNodes;

        // clone the nodes
        for (uint32 i = 0; i < numNodes; ++i)
        {
            result->AddNode(m_nodes[i]->Clone(result));
        }

        result->m_bindPose = m_bindPose;

        return result;
    }


    // reserve memory
    void Skeleton::ReserveNodes(uint32 numNodes)
    {
        m_nodes.Reserve(numNodes);
    }


    // add a node
    void Skeleton::AddNode(Node* node)
    {
        m_nodes.Add(node);
        m_nodesMap[node->GetNameString()] = node;
    }


    // remove a node
    void Skeleton::RemoveNode(uint32 nodeIndex, bool delFromMem)
    {
        m_nodesMap.erase(m_nodes[nodeIndex]->GetNameString());
        if (delFromMem)
        {
            m_nodes[nodeIndex]->Destroy();
        }

        m_nodes.Remove(nodeIndex);
    }


    // remove all nodes
    void Skeleton::RemoveAllNodes(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numNodes = m_nodes.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                m_nodes[i]->Destroy();
            }
        }

        m_nodes.Clear();
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
        const uint32 numNodes = m_nodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (AzFramework::StringFunc::Equal(m_nodes[i]->GetNameString().c_str(), name, false /* no case */))
            {
                return m_nodes[i];
            }
        }

        return nullptr;
    }


    // search for a node on ID
    Node* Skeleton::FindNodeByID(uint32 id) const
    {
        // check the ID's for all nodes
        const uint32 numNodes = m_nodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (m_nodes[i]->GetID() == id)
            {
                return m_nodes[i];
            }
        }

        return nullptr;
    }


    // set a given node
    void Skeleton::SetNode(uint32 index, Node* node)
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
    void Skeleton::SetNumNodes(uint32 numNodes)
    {
        uint32 oldLength = m_nodes.GetLength();
        m_nodes.Resize(numNodes);
        for (uint32 i = oldLength; i < numNodes; ++i)
        {
            m_nodes[i] = nullptr;
        }
        m_bindPose.SetNumTransforms(numNodes);
    }


    // update the node indices
    void Skeleton::UpdateNodeIndexValues(uint32 startNode)
    {
        const uint32 numNodes = m_nodes.GetLength();
        for (uint32 i = startNode; i < numNodes; ++i)
        {
            m_nodes[i]->SetNodeIndex(i);
        }
    }


    // reserve memory for the root nodes array
    void Skeleton::ReserveRootNodes(uint32 numNodes)
    {
        m_rootNodes.Reserve(numNodes);
    }


    // add a root node
    void Skeleton::AddRootNode(uint32 nodeIndex)
    {
        m_rootNodes.Add(nodeIndex);
    }


    // remove a given root node
    void Skeleton::RemoveRootNode(uint32 nr)
    {
        m_rootNodes.Remove(nr);
    }


    // remove all root nodes
    void Skeleton::RemoveAllRootNodes()
    {
        m_rootNodes.Clear();
    }


    // log all node names
    void Skeleton::LogNodes()
    {
        const uint32 numNodes = m_nodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            MCore::LogInfo("%d = '%s'", i, m_nodes[i]->GetName());
        }
    }


    // calculate the hierarchy depth for a given node
    uint32 Skeleton::CalcHierarchyDepthForNode(uint32 nodeIndex) const
    {
        uint32 result = 0;
        Node* curNode = m_nodes[nodeIndex];
        while (curNode->GetParentNode())
        {
            result++;
            curNode = curNode->GetParentNode();
        }

        return result;
    }


    Node* Skeleton::FindNodeAndIndexByName(const AZStd::string& name, AZ::u32& outIndex) const
    {
        if (name.empty())
        {
            outIndex = MCORE_INVALIDINDEX32;
            return nullptr;
        }

        Node* joint = FindNodeByNameNoCase(name.c_str());
        if (!joint)
        {
            outIndex = MCORE_INVALIDINDEX32;
            return nullptr;
        }

        outIndex = joint->GetNodeIndex();
        return joint;
    }
}   // namespace EMotionFX
