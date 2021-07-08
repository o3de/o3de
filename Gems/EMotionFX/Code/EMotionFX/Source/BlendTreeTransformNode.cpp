/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeTransformNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeTransformNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeTransformNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    BlendTreeTransformNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeTransformNode::UniqueData::Update()
    {
        BlendTreeTransformNode* transformNode = azdynamic_cast<BlendTreeTransformNode*>(mObject);
        AZ_Assert(transformNode, "Unique data linked to incorrect node type.");

        const ActorInstance* actorInstance = mAnimGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();

        const AZStd::string& targetJointName = transformNode->GetTargetJointName();

        mNodeIndex = InvalidIndex32;
        SetHasError(true);

        if (!targetJointName.empty())
        {
            const Node* joint = actor->GetSkeleton()->FindNodeByName(targetJointName);
            if (joint)
            {
                mNodeIndex = joint->GetNodeIndex();
                SetHasError(false);
            }
        }
    }

    BlendTreeTransformNode::BlendTreeTransformNode()
        : AnimGraphNode()
        , m_minTranslation(AZ::Vector3::CreateZero())
        , m_maxTranslation(AZ::Vector3::CreateZero())
        , m_minRotation(AZ::Vector3::CreateAxisZ(-180.0f))
        , m_maxRotation(AZ::Vector3::CreateAxisZ(180.0f))
        , m_minScale(AZ::Vector3::CreateZero())
        , m_maxScale(AZ::Vector3::CreateZero())
    {
        // setup the input ports
        InitInputPorts(4);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Translation", INPUTPORT_TRANSLATE_AMOUNT, PORTID_INPUT_TRANSLATE_AMOUNT);
        SetupInputPortAsNumber("Rotation", INPUTPORT_ROTATE_AMOUNT, PORTID_INPUT_ROTATE_AMOUNT);
        SetupInputPortAsNumber("Scale", INPUTPORT_SCALE_AMOUNT, PORTID_INPUT_SCALE_AMOUNT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    BlendTreeTransformNode::~BlendTreeTransformNode()
    {
    }

    bool BlendTreeTransformNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeTransformNode::GetPaletteName() const
    {
        return "Transform";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // perform the calculations / actions
    void BlendTreeTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        AnimGraphPose* outputPose;

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (uniqueData->GetHasError())
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, true);
            }
            return;
        }
        else if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, false);
        }

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
        }
        else // init on the input pose
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *inputPose;
        }

        // get the local transform from our node
        Transform inputTransform = outputPose->GetPose().GetLocalSpaceTransform(uniqueData->mNodeIndex);
        Transform outputTransform = inputTransform;

        // process the rotation
        if (GetInputPort(INPUTPORT_ROTATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ROTATE_AMOUNT));
            const float rotateFactor = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_ROTATE_AMOUNT), 0.0f, 1.0f);
            const AZ::Vector3 newAngles = MCore::LinearInterpolate<AZ::Vector3>(m_minRotation, m_maxRotation, rotateFactor);
            outputTransform.mRotation = inputTransform.mRotation * MCore::AzEulerAnglesToAzQuat(MCore::Math::DegreesToRadians(newAngles.GetX()), 
                MCore::Math::DegreesToRadians(newAngles.GetY()), 
                MCore::Math::DegreesToRadians(newAngles.GetZ()));
        }

        // process the translation
        if (GetInputPort(INPUTPORT_TRANSLATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TRANSLATE_AMOUNT));
            const float factor = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TRANSLATE_AMOUNT), 0.0f, 1.0f);
            const AZ::Vector3 newValue = MCore::LinearInterpolate<AZ::Vector3>(m_minTranslation, m_maxTranslation, factor);
            outputTransform.mPosition = inputTransform.mPosition + newValue;
        }

        // process the scale
        EMFX_SCALECODE
        (
            if (GetInputPort(INPUTPORT_SCALE_AMOUNT).mConnection)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_SCALE_AMOUNT));
                const float factor = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_SCALE_AMOUNT), 0.0f, 1.0f);
                const AZ::Vector3 newValue = MCore::LinearInterpolate<AZ::Vector3>(m_minScale, m_maxScale, factor);
                outputTransform.mScale = inputTransform.mScale + newValue;
            }
        )

        // update the transformation of the node
        outputPose->GetPose().SetLocalSpaceTransform(uniqueData->mNodeIndex, outputTransform);

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    }

    void BlendTreeTransformNode::SetTargetNodeName(const AZStd::string& targetNodeName)
    {
        m_targetNodeName = targetNodeName;
    }

    void BlendTreeTransformNode::SetMinTranslation(const AZ::Vector3& minTranslation)
    {
        m_minTranslation = minTranslation;
    }

    void BlendTreeTransformNode::SetMaxTranslation(const AZ::Vector3& maxTranslation)
    {
        m_maxTranslation = maxTranslation;
    }

    void BlendTreeTransformNode::SetMinRotation(const AZ::Vector3& minRotation)
    {
        m_minRotation = minRotation;
    }

    void BlendTreeTransformNode::SetMaxRotation(const AZ::Vector3& maxRotation)
    {
        m_maxRotation = maxRotation;
    }

    void BlendTreeTransformNode::SetMinScale(const AZ::Vector3& minScale)
    {
        m_minScale = minScale;
    }

    void BlendTreeTransformNode::SetMaxScale(const AZ::Vector3& maxScale)
    {
        m_maxScale = maxScale;
    }

    void BlendTreeTransformNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeTransformNode, AnimGraphNode>()
            ->Version(1)
            ->Field("targetNodeName", &BlendTreeTransformNode::m_targetNodeName)
            ->Field("minTranslation", &BlendTreeTransformNode::m_minTranslation)
            ->Field("maxTranslation", &BlendTreeTransformNode::m_maxTranslation)
            ->Field("minRotation", &BlendTreeTransformNode::m_minRotation)
            ->Field("maxRotation", &BlendTreeTransformNode::m_maxRotation)
            ->Field("minScale", &BlendTreeTransformNode::m_minScale)
            ->Field("maxScale", &BlendTreeTransformNode::m_maxScale)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeTransformNode>("Transform Node", "Transform node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeTransformNode::m_targetNodeName, "Node", "The node to apply the transform to.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTransformNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTransformNode::m_minTranslation, "Min Translation", "The minimum translation value, used when the input translation amount equals zero.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTransformNode::m_maxTranslation, "Max Translation", "The maximum translation value, used when the input translation amount equals one.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTransformNode::m_minRotation, "Min Rotation", "The minimum rotation value, in degrees, used when the input rotation amount equals zero.")
            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTransformNode::m_maxRotation, "Max Rotation", "The maximum rotation value, in degrees, used when the input rotation amount equals one.")
            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTransformNode::m_minScale, "Min Scale", "The minimum scale value, used when the input scale amount equals zero.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTransformNode::m_maxScale, "Max Scale", "The maximum scale value, used when the input scale amount equals one.")
        ;
    }
} // namespace EMotionFX
