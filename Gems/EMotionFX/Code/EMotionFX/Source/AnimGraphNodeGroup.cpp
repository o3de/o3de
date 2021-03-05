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

#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphNodeGroup.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeGroup, AnimGraphAllocator, 0)

    AnimGraphNodeGroup::AnimGraphNodeGroup()
        : mColor(AZ::Color::CreateU32(255, 255, 255, 255))
        , mIsVisible(true)
    {
    }


    AnimGraphNodeGroup::AnimGraphNodeGroup(const char* groupName)
    {
        SetName(groupName);
        mIsVisible = true;
    }


    AnimGraphNodeGroup::AnimGraphNodeGroup(const char* groupName, uint32 numNodes)
    {
        SetName(groupName);
        SetNumNodes(numNodes);
        mIsVisible = true;
    }


    AnimGraphNodeGroup::~AnimGraphNodeGroup()
    {
        RemoveAllNodes();
    }


    void AnimGraphNodeGroup::RemoveAllNodes()
    {
        mNodeIds.clear();
    }


    // set the name of the group
    void AnimGraphNodeGroup::SetName(const char* groupName)
    {
        if (groupName)
        {
            mName = groupName;
        }
        else
        {
            mName.clear();
        }
    }


    // get the name of the group as character buffer
    const char* AnimGraphNodeGroup::GetName() const
    {
        return mName.c_str();
    }


    // get the name of the string as mcore string object
    const AZStd::string& AnimGraphNodeGroup::GetNameString() const
    {
        return mName;
    }


    // set the color of the group
    void AnimGraphNodeGroup::SetColor(const AZ::u32& color)
    {
        mColor = color;
    }


    // get the color of the group
    AZ::u32 AnimGraphNodeGroup::GetColor() const
    {
        return mColor;
    }


    // set the visibility flag
    void AnimGraphNodeGroup::SetIsVisible(bool isVisible)
    {
        mIsVisible = isVisible;
    }


    // set the number of nodes
    void AnimGraphNodeGroup::SetNumNodes(uint32 numNodes)
    {
        mNodeIds.resize(numNodes);
    }


    // get the number of nodes
    uint32 AnimGraphNodeGroup::GetNumNodes() const
    {
        return static_cast<uint32>(mNodeIds.size());
    }


    // set a given node to a given node number
    void AnimGraphNodeGroup::SetNode(uint32 index, AnimGraphNodeId nodeId)
    {
        mNodeIds[index] = nodeId;
    }


    // get the node number of a given index
    AnimGraphNodeId AnimGraphNodeGroup::GetNode(uint32 index) const
    {
        return mNodeIds[index];
    }


    // add a given node to the group (performs a realloc internally)
    void AnimGraphNodeGroup::AddNode(AnimGraphNodeId nodeId)
    {
        // add the node in case it is not in yet
        if (Contains(nodeId) == false)
        {
            mNodeIds.push_back(nodeId);
        }
    }


    // remove a given node by its node id
    void AnimGraphNodeGroup::RemoveNodeById(AnimGraphNodeId nodeId)
    {
        const AZ::u64 convertedId = nodeId;
        mNodeIds.erase(AZStd::remove(mNodeIds.begin(), mNodeIds.end(), convertedId), mNodeIds.end());
    }


    // remove a given array element from the list of nodes
    void AnimGraphNodeGroup::RemoveNodeByGroupIndex(uint32 index)
    {
        mNodeIds.erase(mNodeIds.begin() + index);
    }


    // check if the given node id
    bool AnimGraphNodeGroup::Contains(AnimGraphNodeId nodeId) const
    {
        const AZ::u64 convertedId = nodeId;
        return AZStd::find(mNodeIds.begin(), mNodeIds.end(), convertedId) != mNodeIds.end();
    }


    // init from another group
    void AnimGraphNodeGroup::InitFrom(const AnimGraphNodeGroup& other)
    {
        mNodeIds        = other.mNodeIds;
        mColor          = other.mColor;
        mName           = other.mName;
        mIsVisible      = other.mIsVisible;
    }


    bool AnimGraphNodeGroup::GetIsVisible() const
    {
        return mIsVisible;
    }


    void AnimGraphNodeGroup::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphNodeGroup>()
            ->Version(1)
            ->Field("nodes", &AnimGraphNodeGroup::mNodeIds)
            ->Field("name", &AnimGraphNodeGroup::mName)
            ->Field("color", &AnimGraphNodeGroup::mColor)
            ->Field("isVisible", &AnimGraphNodeGroup::mIsVisible);
    }
} // namespace EMotionFX