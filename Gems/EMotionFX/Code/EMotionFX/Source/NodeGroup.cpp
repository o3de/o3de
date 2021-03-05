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

// include the required headers
#include "NodeGroup.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeGroup, NodeAllocator, 0)


    // default constructor
    NodeGroup::NodeGroup()
        : BaseObject()
    {
        SetIsEnabledOnDefault(true);
    }


    // extended constructor
    NodeGroup::NodeGroup(const char* groupName, bool enabledOnDefault)
        : BaseObject()
    {
        SetName(groupName);
        SetIsEnabledOnDefault(enabledOnDefault);
    }


    // another extended constructor
    NodeGroup::NodeGroup(const char* groupName, uint16 numNodes, bool enabledOnDefault)
        : BaseObject()
    {
        SetName(groupName);
        SetNumNodes(numNodes);
        SetIsEnabledOnDefault(enabledOnDefault);
    }


    // destructor
    NodeGroup::~NodeGroup()
    {
        mNodes.Clear();
    }


    // create
    NodeGroup* NodeGroup::Create()
    {
        return aznew NodeGroup();
    }


    // create
    NodeGroup* NodeGroup::Create(const char* groupName, bool enabledOnDefault)
    {
        return aznew NodeGroup(groupName, enabledOnDefault);
    }


    // create
    NodeGroup* NodeGroup::Create(const char* groupName, uint16 numNodes, bool enabledOnDefault)
    {
        return aznew NodeGroup(groupName, numNodes, enabledOnDefault);
    }


    // set the name of the group
    void NodeGroup::SetName(const char* groupName)
    {
        mName = groupName;
    }


    // get the name of the group as character buffer
    const char* NodeGroup::GetName() const
    {
        return mName.c_str();
    }


    // get the name of the string as mcore string object
    const AZStd::string& NodeGroup::GetNameString() const
    {
        return mName;
    }


    // set the number of nodes
    void NodeGroup::SetNumNodes(const uint16 numNodes)
    {
        mNodes.Resize(numNodes);
    }


    // get the number of nodes
    uint16 NodeGroup::GetNumNodes() const
    {
        return static_cast<uint16>(mNodes.GetLength());
    }


    // set a given node to a given node number
    void NodeGroup::SetNode(uint16 index, uint16 nodeIndex)
    {
        mNodes[index] = nodeIndex;
    }


    // get the node number of a given index
    uint16 NodeGroup::GetNode(uint16 index)
    {
        return mNodes[index];
    }


    // enable all nodes in the group inside a given actor instance
    void NodeGroup::EnableNodes(ActorInstance* targetActorInstance)
    {
        const uint16 numNodes = static_cast<uint16>(mNodes.GetLength());
        for (uint16 i = 0; i < numNodes; ++i)
        {
            targetActorInstance->EnableNode(mNodes[i]);
        }
    }


    // disable all nodes in the group inside a given actor instance
    void NodeGroup::DisableNodes(ActorInstance* targetActorInstance)
    {
        const uint16 numNodes = static_cast<uint16>(mNodes.GetLength());
        for (uint16 i = 0; i < numNodes; ++i)
        {
            targetActorInstance->DisableNode(mNodes[i]);
        }
    }


    // add a given node to the group (performs a realloc internally)
    void NodeGroup::AddNode(uint16 nodeIndex)
    {
        mNodes.Add(nodeIndex);
    }


    // remove a given node by its node number
    void NodeGroup::RemoveNodeByNodeIndex(uint16 nodeIndex)
    {
        mNodes.RemoveByValue(nodeIndex);
    }


    // remove a given array element from the list of nodes
    void NodeGroup::RemoveNodeByGroupIndex(uint16 index)
    {
        mNodes.Remove(index);
    }


    // get the node array directly
    MCore::SmallArray<uint16>& NodeGroup::GetNodeArray()
    {
        return mNodes;
    }


    // is this group enabled on default?
    bool NodeGroup::GetIsEnabledOnDefault() const
    {
        return mEnabledOnDefault;
    }


    // set the default enabled state
    void NodeGroup::SetIsEnabledOnDefault(bool enabledOnDefault)
    {
        mEnabledOnDefault = enabledOnDefault;
    }

    NodeGroup::NodeGroup(const NodeGroup& aOther)
    {
        *this = aOther;
    }

    NodeGroup& NodeGroup::operator=(const NodeGroup& aOther)
    {
        mName = aOther.mName;
        mNodes = aOther.mNodes;
        mEnabledOnDefault = aOther.mEnabledOnDefault;
        return *this;
    }

}   // namespace EMotionFX
