/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AnimAZEntityNode.h"
#include "AnimComponentNode.h"

#include <AzFramework/Components/CameraBus.h>   // for definition of EditorCameraComponentTypeId
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <Maestro/Bus/SequenceComponentBus.h>
#include <Maestro/Types/AnimParamType.h>
#include <Maestro/Types/AnimNodeType.h>

namespace Maestro
{

    CAnimAzEntityNode::CAnimAzEntityNode(const int id)
        : CAnimNode(id, AnimNodeType::AzEntity)
    {
        SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
    }

    CAnimAzEntityNode::CAnimAzEntityNode()
        : CAnimAzEntityNode(0)
    {
    }

    CAnimAzEntityNode::~CAnimAzEntityNode()
    {
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    void CAnimAzEntityNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
        if (bLoading)
        {
            AZ::u64 id64;
            if (xmlNode->getAttr("AnimatedEntityId", id64))
            {
                m_entityId = AZ::EntityId(id64);
            }
        }
        else
        {
            // saving
            if (m_entityId.IsValid())
            {
                AZ::u64 id64 = static_cast<AZ::u64>(m_entityId);
                xmlNode->setAttr("AnimatedEntityId", id64);
            }
        }
    }

    void CAnimAzEntityNode::SetSkipInterpolatedCameraNode(const bool skipNodeCameraAnimation)
    {
        // Skip animations on transforms
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            transformComponentNode->SetSkipComponentAnimationUpdates(skipNodeCameraAnimation);
        }

        // Skip animations on cameras
        CAnimComponentNode* cameraComponentNode = GetComponentNodeForComponentWithTypeId(AZ::Uuid(EditorCameraComponentTypeId));
        if (!cameraComponentNode)
        {
            cameraComponentNode = GetComponentNodeForComponentWithTypeId(AZ::Uuid(CameraComponentTypeId));
        }
        if (cameraComponentNode)
        {
            cameraComponentNode->SetSkipComponentAnimationUpdates(skipNodeCameraAnimation);
        }
    }

    void CAnimAzEntityNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimAzEntityNode, CAnimNode>()->Version(1)->Field("Entity", &CAnimAzEntityNode::m_entityId);
        }
    }

    CAnimParamType CAnimAzEntityNode::GetParamType([[maybe_unused]] unsigned int nIndex) const
    {
        return AnimParamType::Invalid;
    }

    CAnimComponentNode* CAnimAzEntityNode::GetComponentNodeForComponentWithTypeId(const AZ::Uuid& componentTypeId) const
    {
        CAnimComponentNode* retTransformNode = nullptr;

        for (int i = m_pSequence->GetNodeCount(); --i >= 0;)
        {
            IAnimNode* node = m_pSequence->GetNode(i);
            if (node && node->GetParent() == this && node->GetType() == AnimNodeType::Component)
            {
                if (static_cast<CAnimComponentNode*>(node)->GetComponentTypeId() == componentTypeId)
                {
                    retTransformNode = static_cast<CAnimComponentNode*>(node);
                    break;
                }
            }
        }
        return retTransformNode;
    }

    CAnimComponentNode* CAnimAzEntityNode::GetTransformComponentNode() const
    {
        CAnimComponentNode* retTransformNode = GetComponentNodeForComponentWithTypeId(AZ::Uuid(AZ::EditorTransformComponentTypeId));

        if (!retTransformNode)
        {
            // if not Editor transform, try run-time transform
            retTransformNode = GetComponentNodeForComponentWithTypeId(AzFramework::TransformComponent::TYPEINFO_Uuid());
        }
        return retTransformNode;
    }

    void CAnimAzEntityNode::SetPos(float time, const AZ::Vector3& pos)
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            transformComponentNode->SetPos(time, pos);
        }
    }

    Vec3 CAnimAzEntityNode::GetPos()
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            return transformComponentNode->GetPos();
        }
        return Vec3(.0f, .0f, .0f);
    }

    void CAnimAzEntityNode::SetRotate(float time, const AZ::Quaternion& rotation)
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            transformComponentNode->SetRotate(time, rotation);
        }
    }

    Quat CAnimAzEntityNode::GetRotate()
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            return transformComponentNode->GetRotate();
        }
        return Quat::CreateIdentity();
    }

    Quat CAnimAzEntityNode::GetRotate(float time)
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            return transformComponentNode->GetRotate(time);
        }
        return Quat::CreateIdentity();
    }

    void CAnimAzEntityNode::SetScale(float time, const AZ::Vector3& scale)
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            transformComponentNode->SetScale(time, scale);
        }
    }

    Vec3 CAnimAzEntityNode::GetScale()
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            return transformComponentNode->GetScale();
        }

        return Vec3(.0f, .0f, .0f);
    }

    Vec3 CAnimAzEntityNode::GetOffsetPosition(const Vec3& position)
    {
        if (auto transformComponentNode = GetTransformComponentNode())
        {
            return position - transformComponentNode->GetPos();
        }
        return Vec3(.0f, .0f, .0f);
    }

} // namespace Maestro
