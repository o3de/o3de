/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeGroupInfo, EMStudio::UIAllocator)

    NodeGroupInfo::NodeGroupInfo(EMotionFX::Actor* actor, EMotionFX::NodeGroup* nodeGroup)
    {
        m_name = nodeGroup->GetNameString();

        // iterate over the nodes inside the node group
        const size_t numGroupNodes = nodeGroup->GetNumNodes();
        for (size_t j = 0; j < numGroupNodes; ++j)
        {
            const uint16 nodeIndex = nodeGroup->GetNode(static_cast<uint16>(j));
            const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
            m_nodes.emplace_back(node->GetNameString());
        }
    }
    

    void NodeGroupInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<NodeGroupInfo>()
            ->Version(1)
            ->Field("name", &NodeGroupInfo::m_name)
            ->Field("nodes", &NodeGroupInfo::m_nodes)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<NodeGroupInfo>("Node group info", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeGroupInfo::m_name, "Name", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeGroupInfo::m_nodes, "Nodes", "")
            ;
    }

} // namespace EMStudio
