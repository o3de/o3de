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
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/NodeAttribute.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeInfo.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NodeInfo, EMStudio::UIAllocator, 0)

    NodeInfo::NodeInfo(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        const uint32 nodeIndex = node->GetNodeIndex();

        EMotionFX::Actor* actor = actorInstance->GetActor();
        EMotionFX::TransformData* transformData = actorInstance->GetTransformData();

        m_name = node->GetNameString();

        // transform info
        m_position = transformData->GetCurrentPose()->GetLocalSpaceTransform(nodeIndex).mPosition;
        m_rotation = transformData->GetCurrentPose()->GetLocalSpaceTransform(nodeIndex).mRotation;

#ifndef EMFX_SCALE_DISABLED
        m_scale = transformData->GetCurrentPose()->GetLocalSpaceTransform(nodeIndex).mScale;
#else
        m_scale = AZ::Vector3::CreateOne();
#endif

        {
            // parent node
            EMotionFX::Node* parent = node->GetParentNode();
            if (parent)
            {
                m_parentName = parent->GetNameString();
            }
        }

        // the mirrored node
        if (actor->GetHasMirrorInfo())
        {
            const EMotionFX::Actor::NodeMirrorInfo& nodeMirrorInfo = actor->GetNodeMirrorInfo(nodeIndex);
            if (nodeMirrorInfo.mSourceNode != MCORE_INVALIDINDEX16 && nodeMirrorInfo.mSourceNode != nodeIndex)
            {
                m_mirrorNodeName = actor->GetSkeleton()->GetNode(nodeMirrorInfo.mSourceNode)->GetNameString();
            }
        }

        // children
        const uint32 numChildren = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            EMotionFX::Node* child = actor->GetSkeleton()->GetNode(node->GetChildIndex(i));
            m_childNodeNames.emplace_back(child->GetNameString());
        }

        // attributes
        const uint32 numAttributes = node->GetNumAttributes();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            EMotionFX::NodeAttribute* nodeAttribute = node->GetAttribute(i);
            m_attributeTypes.emplace_back(nodeAttribute->GetTypeString());
        }

        // meshes
        const uint32 numLODLevels = actor->GetNumLODLevels();
        for (uint32 i = 0; i < numLODLevels; ++i)
        {
            EMotionFX::Mesh* mesh = actor->GetMesh(i, node->GetNodeIndex());
            if (mesh)
            {
                m_meshByLod.emplace_back(actor, node, i, mesh);
            }
        }
    }

    void NodeInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<NodeInfo>()
            ->Version(1)
            ->Field("name", &NodeInfo::m_name)
            ->Field("position", &NodeInfo::m_position)
            ->Field("rotation", &NodeInfo::m_rotation)
            ->Field("scale", &NodeInfo::m_scale)
            ->Field("parentName", &NodeInfo::m_parentName)
            ->Field("mirrorNodeName", &NodeInfo::m_mirrorNodeName)
            ->Field("childNodeNames", &NodeInfo::m_childNodeNames)
            ->Field("attributeTypes", &NodeInfo::m_attributeTypes)
            ->Field("meshByLod", &NodeInfo::m_meshByLod)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<NodeInfo>("Node info", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_name, "Name", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_position, "Position", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_rotation, "Rotation", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_scale, "Scale", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::Visibility,
#ifndef EMFX_SCALE_DISABLED
            true
#else
            false
#endif
            )
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_parentName, "Parent name", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_mirrorNodeName, "Mirror", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, &NodeInfo::HasMirror)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_childNodeNames, "Child nodes", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, &NodeInfo::HasChildNodes)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->ElementAttribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_attributeTypes, "Attributes", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, &NodeInfo::HasAttributes)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeInfo::m_meshByLod, "Meshes by lod", "")
                ->Attribute(AZ::Edit::Attributes::Visibility, &NodeInfo::HasMeshes)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
        ;
    }
} // namespace EMStudio
