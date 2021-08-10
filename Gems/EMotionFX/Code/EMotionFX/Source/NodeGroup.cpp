/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "NodeGroup.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeGroup, NodeAllocator, 0)


    NodeGroup::NodeGroup(const AZStd::string& groupName, uint16 numNodes, bool enabledOnDefault)
        : m_name(groupName)
        , m_nodes(numNodes)
        , m_enabledOnDefault(enabledOnDefault)
    {
    }


    // set the name of the group
    void NodeGroup::SetName(const AZStd::string& groupName)
    {
        m_name = groupName;
    }


    // get the name of the group as character buffer
    const char* NodeGroup::GetName() const
    {
        return m_name.c_str();
    }


    // get the name of the string as mcore string object
    const AZStd::string& NodeGroup::GetNameString() const
    {
        return m_name;
    }


    // set the number of nodes
    void NodeGroup::SetNumNodes(const uint16 numNodes)
    {
        m_nodes.Resize(numNodes);
    }


    // get the number of nodes
    uint16 NodeGroup::GetNumNodes() const
    {
        return static_cast<uint16>(m_nodes.GetLength());
    }


    // set a given node to a given node number
    void NodeGroup::SetNode(uint16 index, uint16 nodeIndex)
    {
        m_nodes[index] = nodeIndex;
    }


    // get the node number of a given index
    uint16 NodeGroup::GetNode(uint16 index) const
    {
        return m_nodes[index];
    }


    // enable all nodes in the group inside a given actor instance
    void NodeGroup::EnableNodes(ActorInstance* targetActorInstance)
    {
        const uint16 numNodes = static_cast<uint16>(m_nodes.GetLength());
        for (uint16 i = 0; i < numNodes; ++i)
        {
            targetActorInstance->EnableNode(m_nodes[i]);
        }
    }


    // disable all nodes in the group inside a given actor instance
    void NodeGroup::DisableNodes(ActorInstance* targetActorInstance)
    {
        const uint16 numNodes = static_cast<uint16>(m_nodes.GetLength());
        for (uint16 i = 0; i < numNodes; ++i)
        {
            targetActorInstance->DisableNode(m_nodes[i]);
        }
    }


    // add a given node to the group (performs a realloc internally)
    void NodeGroup::AddNode(uint16 nodeIndex)
    {
        m_nodes.Add(nodeIndex);
    }


    // remove a given node by its node number
    void NodeGroup::RemoveNodeByNodeIndex(uint16 nodeIndex)
    {
        m_nodes.RemoveByValue(nodeIndex);
    }


    // remove a given array element from the list of nodes
    void NodeGroup::RemoveNodeByGroupIndex(uint16 index)
    {
        m_nodes.Remove(index);
    }


    // get the node array directly
    MCore::SmallArray<uint16>& NodeGroup::GetNodeArray()
    {
        return m_nodes;
    }


    // is this group enabled on default?
    bool NodeGroup::GetIsEnabledOnDefault() const
    {
        return m_enabledOnDefault;
    }


    // set the default enabled state
    void NodeGroup::SetIsEnabledOnDefault(bool enabledOnDefault)
    {
        m_enabledOnDefault = enabledOnDefault;
    }

    NodeGroup::NodeGroup(const NodeGroup& aOther)
    {
        *this = aOther;
    }

    NodeGroup& NodeGroup::operator=(const NodeGroup& aOther)
    {
        m_name = aOther.m_name;
        m_nodes = aOther.m_nodes;
        m_enabledOnDefault = aOther.m_enabledOnDefault;
        return *this;
    }

}   // namespace EMotionFX
