/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "AnimGraphBindPoseNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphBindPoseNode, AnimGraphAllocator)

    AnimGraphBindPoseNode::AnimGraphBindPoseNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    AnimGraphBindPoseNode::~AnimGraphBindPoseNode()
    {
    }


    bool AnimGraphBindPoseNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    // get the palette name
    const char* AnimGraphBindPoseNode::GetPaletteName() const
    {
        return "Bind Pose";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphBindPoseNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // perform the calculations / actions
    void AnimGraphBindPoseNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // get the output pose
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    void AnimGraphBindPoseNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphBindPoseNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphBindPoseNode>("Bind Pose", "Bind pose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX
