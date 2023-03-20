/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <EMotionFX/Source/BlendTreeRaycastNode.h>
#include <Integration/AnimationBus.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRaycastNode, AnimGraphAllocator)

    BlendTreeRaycastNode::BlendTreeRaycastNode()
        : AnimGraphNode()
    {
        // Setup the input ports.
        InitInputPorts(2);
        SetupInputPort("Ray Start", INPUTPORT_RAY_START, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_RAY_START);
        SetupInputPort("Ray End", INPUTPORT_RAY_END, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_RAY_END);

        // Setup the output ports.
        InitOutputPorts(3);
        SetupOutputPort("Position", OUTPUTPORT_POSITION, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_POSITION);
        SetupOutputPort("Normal", OUTPUTPORT_NORMAL, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_NORMAL);
        SetupOutputPort("Intersected", OUTPUTPORT_INTERSECTED, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_INTERSECTED);

        if (m_animGraph)
        {
            Reinit();
        }
    }

    BlendTreeRaycastNode::~BlendTreeRaycastNode()
    {
    }

    bool BlendTreeRaycastNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }

    const char* BlendTreeRaycastNode::GetPaletteName() const
    {
        return "Raycast";
    }

    AnimGraphObject::ECategory BlendTreeRaycastNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MISC;
    }

    void BlendTreeRaycastNode::DoOutput(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphObjectData* uniqueData = animGraphInstance->FindOrCreateUniqueObjectData(this);

        // Get the ray start and end.
        AZ::Vector3 rayStart;
        AZ::Vector3 rayEnd;
        if (!TryGetInputVector3(animGraphInstance, INPUTPORT_RAY_START, rayStart) || 
            !TryGetInputVector3(animGraphInstance, INPUTPORT_RAY_END, rayEnd))
        {
            SetHasError(uniqueData, true);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_POSITION)->SetValue(AZ::Vector3(0.0f, 0.0f, 0.0f));
            GetOutputVector3(animGraphInstance, OUTPUTPORT_NORMAL)->SetValue(AZ::Vector3(0.0f, 0.0f, 1.0f));
            GetOutputFloat(animGraphInstance, OUTPUTPORT_INTERSECTED)->SetValue(0.0f);
            return;
        }

        SetHasError(uniqueData, false);

        AZ::Vector3 rayDirection = rayEnd - rayStart;
        const float maxDistance = rayDirection.GetLength();
        if (maxDistance > 0.0f)
        {
            rayDirection /= maxDistance;
        }

        Integration::IRaycastRequests::RaycastRequest rayRequest;
        rayRequest.m_start      = rayStart;
        rayRequest.m_direction  = rayDirection;
        rayRequest.m_distance   = maxDistance;
        rayRequest.m_queryType  = AzPhysics::SceneQuery::QueryType::Static;
        rayRequest.m_hint       = Integration::IRaycastRequests::UsecaseHint::Generic;


        // Cast a ray, check for intersections.
        Integration::IRaycastRequests::RaycastResult rayResult;
        if (animGraphInstance->GetActorInstance()->GetIsOwnedByRuntime())
        {
            rayResult = AZ::Interface<Integration::IRaycastRequests>::Get()->Raycast(
                animGraphInstance->GetActorInstance()->GetEntityId(), rayRequest);
        }

        // Set the output values.
        if (rayResult.m_intersected)
        {
            GetOutputVector3(animGraphInstance, OUTPUTPORT_POSITION)->SetValue(rayResult.m_position);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_NORMAL)->SetValue(rayResult.m_normal);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_INTERSECTED)->SetValue(1.0);
        }
        else
        {
            GetOutputVector3(animGraphInstance, OUTPUTPORT_POSITION)->SetValue(rayStart);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_NORMAL)->SetValue(AZ::Vector3(0.0f, 0.0f, 1.0f));
            GetOutputFloat(animGraphInstance, OUTPUTPORT_INTERSECTED)->SetValue(0.0f);
        }
    }

    void BlendTreeRaycastNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        DoOutput(animGraphInstance);
    }

    void BlendTreeRaycastNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRaycastNode, AnimGraphNode>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRaycastNode>("Raycast", "Raycast node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX
