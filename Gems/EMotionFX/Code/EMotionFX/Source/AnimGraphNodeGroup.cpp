/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        : m_color(AZ::Color::CreateU32(255, 255, 255, 255))
        , m_isVisible(true)
    {
    }


    AnimGraphNodeGroup::AnimGraphNodeGroup(const char* groupName)
    {
        SetName(groupName);
        m_isVisible = true;
        m_nameEditOngoing = false;
    }


    AnimGraphNodeGroup::AnimGraphNodeGroup(const char* groupName, size_t numNodes)
    {
        SetName(groupName);
        SetNumNodes(numNodes);
        m_isVisible = true;
        m_nameEditOngoing = false;
    }


    AnimGraphNodeGroup::~AnimGraphNodeGroup()
    {
        RemoveAllNodes();
    }


    void AnimGraphNodeGroup::RemoveAllNodes()
    {
        m_nodeIds.clear();
    }


    // set the name of the group
    void AnimGraphNodeGroup::SetName(const char* groupName)
    {
        if (groupName)
        {
            m_name = groupName;
        }
        else
        {
            m_name.clear();
        }
    }


    // get the name of the group as character buffer
    const char* AnimGraphNodeGroup::GetName() const
    {
        return m_name.c_str();
    }


    // get the name of the string as mcore string object
    const AZStd::string& AnimGraphNodeGroup::GetNameString() const
    {
        return m_name;
    }


    // set the color of the group
    void AnimGraphNodeGroup::SetColor(const AZ::u32& color)
    {
        m_color = color;
    }


    // get the color of the group
    AZ::u32 AnimGraphNodeGroup::GetColor() const
    {
        return m_color;
    }


    // set the visibility flag
    void AnimGraphNodeGroup::SetIsVisible(bool isVisible)
    {
        m_isVisible = isVisible;
    }


    // set the number of nodes
    void AnimGraphNodeGroup::SetNumNodes(size_t numNodes)
    {
        m_nodeIds.resize(numNodes);
    }


    // get the number of nodes
    size_t AnimGraphNodeGroup::GetNumNodes() const
    {
        return m_nodeIds.size();
    }


    // set a given node to a given node number
    void AnimGraphNodeGroup::SetNode(size_t index, AnimGraphNodeId nodeId)
    {
        m_nodeIds[index] = nodeId;
    }


    // get the node number of a given index
    AnimGraphNodeId AnimGraphNodeGroup::GetNode(size_t index) const
    {
        return m_nodeIds[index];
    }


    // add a given node to the group (performs a realloc internally)
    void AnimGraphNodeGroup::AddNode(AnimGraphNodeId nodeId)
    {
        // add the node in case it is not in yet
        if (Contains(nodeId) == false)
        {
            m_nodeIds.push_back(nodeId);
        }
    }


    // remove a given node by its node id
    void AnimGraphNodeGroup::RemoveNodeById(AnimGraphNodeId nodeId)
    {
        const AZ::u64 convertedId = nodeId;
        m_nodeIds.erase(AZStd::remove(m_nodeIds.begin(), m_nodeIds.end(), convertedId), m_nodeIds.end());
    }


    // remove a given array element from the list of nodes
    void AnimGraphNodeGroup::RemoveNodeByGroupIndex(size_t index)
    {
        m_nodeIds.erase(m_nodeIds.begin() + index);
    }


    // check if the given node id
    bool AnimGraphNodeGroup::Contains(AnimGraphNodeId nodeId) const
    {
        const AZ::u64 convertedId = nodeId;
        return AZStd::find(m_nodeIds.begin(), m_nodeIds.end(), convertedId) != m_nodeIds.end();
    }


    // init from another group
    void AnimGraphNodeGroup::InitFrom(const AnimGraphNodeGroup& other)
    {
        m_nodeIds = other.m_nodeIds;
        m_color = other.m_color;
        m_name = other.m_name;
        m_isVisible = other.m_isVisible;
        m_nameEditOngoing = other.m_nameEditOngoing;
    }


    bool AnimGraphNodeGroup::GetIsVisible() const
    {
        return m_isVisible;
    }

    void AnimGraphNodeGroup::SetNameEditOngoing(bool nameEditOngoing)
    {
        m_nameEditOngoing = nameEditOngoing;
    }

    bool AnimGraphNodeGroup::IsNameEditOngoing() const
    {
        return m_nameEditOngoing;
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
            ->Field("nodes", &AnimGraphNodeGroup::m_nodeIds)
            ->Field("name", &AnimGraphNodeGroup::m_name)
            ->Field("color", &AnimGraphNodeGroup::m_color)
            ->Field("isVisible", &AnimGraphNodeGroup::m_isVisible)
            ->Field("isNameEditOngoing", &AnimGraphNodeGroup::m_nameEditOngoing);
    }
} // namespace EMotionFX
