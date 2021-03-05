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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include <EMotionFX/Source/BlendTreeAccumTransformNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphManager.h"
#include "Node.h"
#include "EMotionFXManager.h"
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeAccumTransformNode, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeAccumTransformNode::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    BlendTreeAccumTransformNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
        mAdditiveTransform.Identity();
        EMFX_SCALECODE(mAdditiveTransform.mScale.CreateZero();)
        SetHasError(true);
    }

    void BlendTreeAccumTransformNode::UniqueData::Update()
    {
        BlendTreeAccumTransformNode* accumTransformNode = azdynamic_cast<BlendTreeAccumTransformNode*>(mObject);
        AZ_Assert(accumTransformNode, "Unique data linked to incorrect node type.");

        const ActorInstance* actorInstance = mAnimGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const Node* node = skeleton->FindNodeByName(accumTransformNode->GetTargetNodeName().c_str());
        if (node)
        {
            mNodeIndex = node->GetNodeIndex();
            SetHasError(false);
        }
        else
        {
            mNodeIndex = InvalidIndex32;
            SetHasError(true);
        }
    }

    BlendTreeAccumTransformNode::BlendTreeAccumTransformNode()
        : AnimGraphNode()
        , m_translateSpeed(1.0f)
        , m_rotateSpeed(1.0f)
        , m_scaleSpeed(1.0f)
        , m_translationAxis(AXIS_X)
        , m_rotationAxis(AXIS_X)
        , m_scaleAxis(SCALEAXIS_ALL)
        , m_invertTranslation(false)
        , m_invertRotation(false)
        , m_invertScale(false)
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


    BlendTreeAccumTransformNode::~BlendTreeAccumTransformNode()
    {
    }


    bool BlendTreeAccumTransformNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeAccumTransformNode::GetPaletteName() const
    {
        return "AccumTransform";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeAccumTransformNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // perform the calculations / actions
    void BlendTreeAccumTransformNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        AnimGraphPose* outputPose = nullptr;

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
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
        }

        // get the local transform from our node
        Transform inputTransform;
        outputPose->GetPose().GetLocalSpaceTransform(uniqueData->mNodeIndex, &inputTransform);

        Transform outputTransform = inputTransform;

        // process the rotation
        if (GetInputPort(INPUTPORT_ROTATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ROTATE_AMOUNT));

            const float inputAmount = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_ROTATE_AMOUNT), 0.0f, 1.0f);
            const float factor = m_rotateSpeed;
            float invertFactor = 1.0f;
            if (m_invertRotation)
            {
                invertFactor = -1.0f;
            }

            AZ::Vector3 axis(1.0f, 0.0f, 0.0f);
            switch (m_rotationAxis)
            {
            case AXIS_X:
                axis.Set(1.0f, 0.0f, 0.0f);
                break;
            case AXIS_Y:
                axis.Set(0.0f, 1.0f, 0.0f);
                break;
            case AXIS_Z:
                axis.Set(0.0f, 0.0f, 1.0f);
                break;
            default:
                MCORE_ASSERT(false);
            }

            const AZ::Quaternion targetRot = MCore::CreateFromAxisAndAngle(axis, MCore::Math::DegreesToRadians(360.0f * (inputAmount - 0.5f) * invertFactor));
            AZ::Quaternion deltaRot = MCore::LinearInterpolate<AZ::Quaternion>(AZ::Quaternion::CreateIdentity(), targetRot, uniqueData->mDeltaTime * factor);
            deltaRot.Normalize();
            uniqueData->mAdditiveTransform.mRotation = uniqueData->mAdditiveTransform.mRotation * deltaRot;
            outputTransform.mRotation = (inputTransform.mRotation * uniqueData->mAdditiveTransform.mRotation);
            outputTransform.mRotation.Normalize();
        }

        // process the translation
        if (GetInputPort(INPUTPORT_TRANSLATE_AMOUNT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TRANSLATE_AMOUNT));

            const float inputAmount = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_TRANSLATE_AMOUNT), 0.0f, 1.0f);
            const float factor = m_translateSpeed;
            float invertFactor = 1.0f;
            if (m_invertTranslation)
            {
                invertFactor = -1.0f;
            }

            AZ::Vector3 axis(1.0f, 0.0f, 0.0f);
            switch (m_translationAxis)
            {
            case AXIS_X:
                axis.Set(1.0f, 0.0f, 0.0f);
                break;
            case AXIS_Y:
                axis.Set(0.0f, 1.0f, 0.0f);
                break;
            case AXIS_Z:
                axis.Set(0.0f, 0.0f, 1.0f);
                break;
            default:
                MCORE_ASSERT(false);
            }

            axis *= (inputAmount - 0.5f) * invertFactor;
            uniqueData->mAdditiveTransform.mPosition += MCore::LinearInterpolate<AZ::Vector3>(AZ::Vector3::CreateZero(), axis, uniqueData->mDeltaTime * factor);
            outputTransform.mPosition = inputTransform.mPosition + uniqueData->mAdditiveTransform.mPosition;
        }


        // process the scale
        EMFX_SCALECODE
        (
            if (GetInputPort(INPUTPORT_SCALE_AMOUNT).mConnection)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_SCALE_AMOUNT));

                const float inputAmount = MCore::Clamp<float>(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_SCALE_AMOUNT), 0.0f, 1.0f);
                const float factor = m_scaleSpeed;
                float invertFactor = 1.0f;
                if (m_invertScale)
                {
                    invertFactor = -1.0f;
                }

                AZ::Vector3 axis(1.0f, 1.0f, 1.0f);
                switch (m_scaleAxis)
                {
                case SCALEAXIS_X:
                    axis.Set(1.0f, 0.0f, 0.0f);
                    break;
                case SCALEAXIS_Y:
                    axis.Set(0.0f, 1.0f, 0.0f);
                    break;
                case SCALEAXIS_Z:
                    axis.Set(0.0f, 0.0f, 1.0f);
                    break;
                case SCALEAXIS_ALL:
                    axis.Set(1.0f, 1.0f, 1.0f);
                    break;
                default:
                    MCORE_ASSERT(false);
                }

                axis *= (inputAmount - 0.5f) * invertFactor;
                uniqueData->mAdditiveTransform.mScale += MCore::LinearInterpolate<AZ::Vector3>(AZ::Vector3::CreateZero(), axis, uniqueData->mDeltaTime * factor);
                outputTransform.mScale = inputTransform.mScale + uniqueData->mAdditiveTransform.mScale;
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

    // update
    void BlendTreeAccumTransformNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all incoming nodes
        AnimGraphNode::Update(animGraphInstance, timePassedInSeconds);

        // store the passed time
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        uniqueData->mDeltaTime = timePassedInSeconds;
    }


    void BlendTreeAccumTransformNode::OnAxisChanged()
    {
        if (!mAnimGraph)
        {
            return;
        }

        const size_t numInstances = mAnimGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = mAnimGraph->GetAnimGraphInstance(i);

            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueNodeData(this));
            if (!uniqueData)
            {
                continue;
            }

            uniqueData->mAdditiveTransform.Identity();
            EMFX_SCALECODE(uniqueData->mAdditiveTransform.mScale.CreateZero();)

            InvalidateUniqueData(animGraphInstance);
        }
    }

    void BlendTreeAccumTransformNode::SetInvertTranslation(bool invertTranslation)
    {
        m_invertTranslation = invertTranslation;
    }

    void BlendTreeAccumTransformNode::SetInvertRotation(bool invertRotation)
    {
        m_invertRotation = invertRotation;
    }

    void BlendTreeAccumTransformNode::SetInvertScale(bool invertScale)
    {
        m_invertScale = invertScale;
    }

    void BlendTreeAccumTransformNode::SetScaleSpeed(float speed)
    {
        m_scaleSpeed = speed;
    }

    void BlendTreeAccumTransformNode::SetRotateSpeed(float speed)
    {
        m_rotateSpeed = speed;
    }

    void BlendTreeAccumTransformNode::SetTranslateSpeed(float speed)
    {
        m_translateSpeed = speed;
    }

    void BlendTreeAccumTransformNode::SetScaleAxis(ScaleAxis axis)
    {
        m_scaleAxis = axis;
    }

    void BlendTreeAccumTransformNode::SetTranslationAxis(Axis axis)
    {
        m_translationAxis = axis;
    }

    void BlendTreeAccumTransformNode::SetRotationAxis(Axis axis)
    {
        m_rotationAxis = axis;
    }

    void BlendTreeAccumTransformNode::SetTargetNodeName(const AZStd::string& targetNodeName)
    {
        m_targetNodeName = targetNodeName;
    }

    void BlendTreeAccumTransformNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeAccumTransformNode, AnimGraphNode>()
            ->Version(1)
            ->Field("targetNodeName", &BlendTreeAccumTransformNode::m_targetNodeName)
            ->Field("translateAxis", &BlendTreeAccumTransformNode::m_translationAxis)
            ->Field("rotationAxis", &BlendTreeAccumTransformNode::m_rotationAxis)
            ->Field("scaleAxis", &BlendTreeAccumTransformNode::m_scaleAxis)
            ->Field("translateSpeed", &BlendTreeAccumTransformNode::m_translateSpeed)
            ->Field("rotateSpeed", &BlendTreeAccumTransformNode::m_rotateSpeed)
            ->Field("scaleSpeed", &BlendTreeAccumTransformNode::m_scaleSpeed)
            ->Field("invertTranslation", &BlendTreeAccumTransformNode::m_invertTranslation)
            ->Field("invertRotation", &BlendTreeAccumTransformNode::m_invertRotation)
            ->Field("invertScale", &BlendTreeAccumTransformNode::m_invertScale)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeAccumTransformNode>("Bool Logic", "Bool logic attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("ActorNode", 0x35d9eb50), &BlendTreeAccumTransformNode::m_targetNodeName, "Node", "The node to apply the transform to.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeAccumTransformNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeAccumTransformNode::m_translationAxis, "Translation Axis", "The local axis to translate along.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeAccumTransformNode::OnAxisChanged)
            ->EnumAttribute(AXIS_X, "X Axis")
            ->EnumAttribute(AXIS_Y, "Y Axis")
            ->EnumAttribute(AXIS_Z, "Z Axis")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeAccumTransformNode::m_rotationAxis, "Rotation Axis", "The local axis to rotate around.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeAccumTransformNode::OnAxisChanged)
            ->EnumAttribute(AXIS_X, "X Axis")
            ->EnumAttribute(AXIS_Y, "Y Axis")
            ->EnumAttribute(AXIS_Z, "Z Axis")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeAccumTransformNode::m_scaleAxis, "Scaling Axis", "The local axis to scale along.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeAccumTransformNode::OnAxisChanged)
            ->EnumAttribute(SCALEAXIS_X, "X Axis")
            ->EnumAttribute(SCALEAXIS_Y, "Y Axis")
            ->EnumAttribute(SCALEAXIS_Z, "Z Axis")
            ->EnumAttribute(SCALEAXIS_ALL, "All Axes (uniform scaling)")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeAccumTransformNode::m_translateSpeed, "Translate Speed", "The translation speed factor.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeAccumTransformNode::m_rotateSpeed, "Rotate Speed", "The rotation speed factor.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeAccumTransformNode::m_scaleSpeed, "Scale Speed", "The scale speed factor.")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &BlendTreeAccumTransformNode::m_invertTranslation, "Invert Translation", "Invert the translation direction?")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &BlendTreeAccumTransformNode::m_invertRotation, "Invert Rotation", "Invert the rotation direction?")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &BlendTreeAccumTransformNode::m_invertScale, "Invert Scaling", "Invert the scale direction?")
        ;
    }
} // namespace EMotionFX
