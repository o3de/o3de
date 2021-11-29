/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeGroup.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeGroup, NodeAllocator, 0)


    NodeGroup::NodeGroup(const AZStd::string& groupName, size_t numNodes, bool enabledOnDefault)
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
    void NodeGroup::SetNumNodes(const size_t numNodes)
    {
        m_nodes.resize(numNodes);
    }


    // get the number of nodes
    size_t NodeGroup::GetNumNodes() const
    {
        return m_nodes.size();
    }


    // set a given node to a given node number
    void NodeGroup::SetNode(size_t index, uint16 nodeIndex)
    {
        m_nodes[index] = nodeIndex;
    }


    // get the node number of a given index
    uint16 NodeGroup::GetNode(size_t index) const
    {
        return m_nodes[index];
    }


    // enable all nodes in the group inside a given actor instance
    void NodeGroup::EnableNodes(ActorInstance* targetActorInstance)
    {
        for (uint16 node : m_nodes)
        {
            targetActorInstance->EnableNode(node);
        }
    }


    // disable all nodes in the group inside a given actor instance
    void NodeGroup::DisableNodes(ActorInstance* targetActorInstance)
    {
        for (uint16 node : m_nodes)
        {
            targetActorInstance->DisableNode(node);
        }
    }


    // add a given node to the group (performs a realloc internally)
    void NodeGroup::AddNode(uint16 nodeIndex)
    {
        m_nodes.emplace_back(nodeIndex);
    }


    // remove a given node by its node number
    void NodeGroup::RemoveNodeByNodeIndex(uint16 nodeIndex)
    {
        m_nodes.erase(AZStd::remove(m_nodes.begin(), m_nodes.end(), nodeIndex), m_nodes.end());
    }


    // remove a given array element from the list of nodes
    void NodeGroup::RemoveNodeByGroupIndex(size_t index)
    {
        m_nodes.erase(AZStd::next(begin(m_nodes), index));
    }


    // get the node array directly
    AZStd::vector<uint16>& NodeGroup::GetNodeArray()
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
