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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeGroupInfo, EMStudio::UIAllocator, 0)

    NodeGroupInfo::NodeGroupInfo(EMotionFX::Actor* actor, EMotionFX::NodeGroup* nodeGroup)
    {
        m_name = nodeGroup->GetNameString();

        // iterate over the nodes inside the node group
        const uint32 numGroupNodes = nodeGroup->GetNumNodes();
        for (uint32 j = 0; j < numGroupNodes; ++j)
        {
            const uint16 nodeIndex = nodeGroup->GetNode(j);
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
