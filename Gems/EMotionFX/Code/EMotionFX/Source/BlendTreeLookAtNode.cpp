/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeLookAtNode.h>
#include <EMotionFX/Source/ConstraintTransformRotationAngles.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/Compare.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeLookAtNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeLookAtNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeLookAtNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeLookAtNode::UniqueData::Update()
    {
        BlendTreeLookAtNode* lookAtNode = azdynamic_cast<BlendTreeLookAtNode*>(m_object);
        AZ_Assert(lookAtNode, "Unique data linked to incorrect node type.");

        const ActorInstance* actorInstance = m_animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();

        m_nodeIndex = InvalidIndex;
        SetHasError(true);

        const AZStd::string& targetJointName = lookAtNode->GetTargetNodeName();
        if (!targetJointName.empty())
        {
            const Node* targetNode = actor->GetSkeleton()->FindNodeByName(targetJointName);
            if (targetNode)
            {
                m_nodeIndex = targetNode->GetNodeIndex();
                SetHasError(false);
            }
        }
    }

    BlendTreeLookAtNode::BlendTreeLookAtNode()
        : AnimGraphNode()
        , m_constraintRotation(AZ::Quaternion::CreateIdentity())
        , m_postRotation(AZ::Quaternion::CreateIdentity())
        , m_limitMin(-90.0f, -50.0f)
        , m_limitMax(90.0f, 30.0f)
        , m_followSpeed(0.75f)
        , m_twistAxis(ConstraintTransformRotationAngles::AXIS_Y)
        , m_limitsEnabled(false)
        , m_smoothing(true)
    {
        // setup the input ports
        InitInputPorts(3);
        SetupInputPort("Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsVector3("Goal Pos", INPUTPORT_GOALPOS, PORTID_INPUT_GOALPOS);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    BlendTreeLookAtNode::~BlendTreeLookAtNode()
    {
    }

    bool BlendTreeLookAtNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeLookAtNode::GetPaletteName() const
    {
        return "LookAt";
    }

    // get the category
    AnimGraphObject::ECategory BlendTreeLookAtNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }

    // the main process method of the final node
    void BlendTreeLookAtNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).m_connection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // get the weight
        float weight = 1.0f;
        if (GetInputPort(INPUTPORT_WEIGHT).m_connection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_WEIGHT));
            weight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);
            weight = MCore::Clamp<float>(weight, 0.0f, 1.0f);
        }

        // if the weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || m_disabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            uniqueData->m_firstUpdate = true;
            return;
        }

        // get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (uniqueData->GetHasError())
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                InvalidateUniqueData(animGraphInstance);
                SetHasError(uniqueData, true);
            }
            return;
        }

        // there is no error
        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, false);
        }



        // get the goal
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALPOS));
        AZ::Vector3 goal = AZ::Vector3::CreateZero();
        TryGetInputVector3(animGraphInstance, INPUTPORT_GOALPOS, goal);

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get a shortcut to the local transform object
        const size_t nodeIndex = uniqueData->m_nodeIndex;

        Pose& outTransformPose = outputPose->GetPose();
        Transform globalTransform = outTransformPose.GetWorldSpaceTransform(nodeIndex);

        Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();

        // Prevent invalid float values inside the LookAt matrix construction when both position and goal are the same
        const AZ::Vector3 diff = globalTransform.m_position - goal;
        if (diff.GetLengthSq() < AZ::Constants::FloatEpsilon)
        {
            goal += AZ::Vector3(0.0f, 0.000001f, 0.0f);
        }

        // calculate the lookat transform
        // TODO: a quaternion lookat function would be nicer, so that there are no matrix operations involved
        AZ::Matrix4x4 lookAt;
        MCore::LookAt(lookAt, globalTransform.m_position, goal, AZ::Vector3(0.0f, 0.0f, 1.0f));
        AZ::Quaternion destRotation = AZ::Quaternion::CreateFromMatrix4x4(lookAt.GetTranspose());

        // apply the post rotation
        destRotation = destRotation * m_postRotation;

        if (m_limitsEnabled)
        {
            // calculate the delta between the bind pose rotation and current target rotation and constraint that to our limits
            const size_t parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
            AZ::Quaternion parentRotationGlobal;
            AZ::Quaternion bindRotationLocal;
            if (parentIndex != InvalidIndex)
            {
                parentRotationGlobal = inputPose->GetPose().GetWorldSpaceTransform(parentIndex).m_rotation;
                bindRotationLocal = actorInstance->GetTransformData()->GetBindPose()->GetLocalSpaceTransform(parentIndex).m_rotation;
            }
            else
            {
                parentRotationGlobal = AZ::Quaternion::CreateIdentity();
                bindRotationLocal = AZ::Quaternion::CreateIdentity();
            }

            const AZ::Quaternion destRotationLocal = destRotation * parentRotationGlobal.GetConjugate();
            const AZ::Quaternion deltaRotLocal     = destRotationLocal * bindRotationLocal.GetConjugate();

            // setup the constraint and execute it
            // the limits are relative to the bind pose in local space
            ConstraintTransformRotationAngles constraint; // TODO: place this inside the unique data? would be faster, but takes more memory, or modify the constraint to support internal modification of a transform directly
            constraint.SetMinRotationAngles(m_limitMin);
            constraint.SetMaxRotationAngles(m_limitMax);
            constraint.SetMinTwistAngle(0.0f);
            constraint.SetMaxTwistAngle(0.0f);
            constraint.SetTwistAxis(m_twistAxis);
            constraint.GetTransform().m_rotation = (deltaRotLocal * m_constraintRotation.GetConjugate());
            constraint.Execute();

            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                AZ::Transform offset = AZ::Transform::CreateFromQuaternion(m_postRotation.GetInverseFull() * bindRotationLocal * m_constraintRotation * parentRotationGlobal);
                offset.SetTranslation(globalTransform.m_position);
                constraint.DebugDraw(actorInstance, offset, GetVisualizeColor(), 0.5f);
            }

            // convert back into world space
            destRotation = (bindRotationLocal * (constraint.GetTransform().m_rotation * m_constraintRotation)) * parentRotationGlobal;
        }

        // init the rotation quaternion to the initial rotation
        if (uniqueData->m_firstUpdate)
        {
            uniqueData->m_rotationQuat = destRotation;
            uniqueData->m_firstUpdate = false;
        }

        // interpolate between the current rotation and the destination rotation
        if (m_smoothing)
        {
            const float speed = m_followSpeed * uniqueData->m_timeDelta * 10.0f;
            if (speed < 1.0f)
            {
                uniqueData->m_rotationQuat = uniqueData->m_rotationQuat.Slerp(destRotation, speed);
            }
            else
            {
                uniqueData->m_rotationQuat = destRotation;
            }
        }
        else
        {
            uniqueData->m_rotationQuat = destRotation;
        }

        uniqueData->m_rotationQuat.Normalize();
        globalTransform.m_rotation = uniqueData->m_rotationQuat;

        // only blend when needed
        if (weight < 0.999f)
        {
            outTransformPose.SetWorldSpaceTransform(nodeIndex, globalTransform);

            const Pose& inputTransformPose = inputPose->GetPose();
            Transform finalTransform = inputTransformPose.GetLocalSpaceTransform(nodeIndex);
            finalTransform.Blend(outTransformPose.GetLocalSpaceTransform(nodeIndex), weight);

            outTransformPose.SetLocalSpaceTransform(nodeIndex, finalTransform);
        }
        else
        {
            outTransformPose.SetWorldSpaceTransform(nodeIndex, globalTransform);
        }

        // perform some debug rendering
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            const float s = animGraphInstance->GetVisualizeScale() * actorInstance->GetVisualizeScale();

            DebugDraw& debugDraw = GetDebugDraw();
            DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(animGraphInstance->GetActorInstance());
            drawData->Lock();
            drawData->DrawLine(goal - AZ::Vector3(s, 0, 0), goal + AZ::Vector3(s, 0, 0), m_visualizeColor);
            drawData->DrawLine(goal - AZ::Vector3(0, s, 0), goal + AZ::Vector3(0, s, 0), m_visualizeColor);
            drawData->DrawLine(goal - AZ::Vector3(0, 0, s), goal + AZ::Vector3(0, 0, s), m_visualizeColor);
            drawData->DrawLine(globalTransform.m_position, goal, m_visualizeColor);
            drawData->DrawLine(globalTransform.m_position, globalTransform.m_position + MCore::CalcUpAxis(globalTransform.m_rotation) * s * 50.0f, AZ::Color(0.0f, 0.0f, 1.0f, 1.0f));
            drawData->Unlock();
        }
    }

    // update
    void BlendTreeLookAtNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the weight node
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            UpdateIncomingNode(animGraphInstance, weightNode, timePassedInSeconds);
        }

        // update the goal node
        AnimGraphNode* goalNode = GetInputNode(INPUTPORT_GOALPOS);
        if (goalNode)
        {
            UpdateIncomingNode(animGraphInstance, goalNode, timePassedInSeconds);
        }

        // update the pose node
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE);
        if (inputNode)
        {
            UpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);

            // update the sync track
            uniqueData->Init(animGraphInstance, inputNode);
        }
        else
        {
            uniqueData->Clear();
        }

        uniqueData->m_timeDelta = timePassedInSeconds;
    }

    AZ::Crc32 BlendTreeLookAtNode::GetLimitWidgetsVisibility() const
    {
        return m_limitsEnabled ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 BlendTreeLookAtNode::GetFollowSpeedVisibility() const
    {
        return m_smoothing ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void BlendTreeLookAtNode::SetTargetNodeName(const AZStd::string& targetNodeName)
    {
        m_targetNodeName = targetNodeName;
    }

    void BlendTreeLookAtNode::SetConstraintRotation(const AZ::Quaternion& constraintRotation)
    {
        m_constraintRotation = constraintRotation;
    }

    void BlendTreeLookAtNode::SetPostRotation(const AZ::Quaternion& postRotation)
    {
        m_postRotation = postRotation;
    }

    void BlendTreeLookAtNode::SetLimitMin(const AZ::Vector2& limitMin)
    {
        m_limitMin = limitMin;
    }

    void BlendTreeLookAtNode::SetLimitMax(const AZ::Vector2& limitMax)
    {
        m_limitMax = limitMax;
    }

    void BlendTreeLookAtNode::SetFollowSpeed(float followSpeed)
    {
        m_followSpeed = followSpeed;
    }

    void BlendTreeLookAtNode::SetTwistAxis(ConstraintTransformRotationAngles::EAxis twistAxis)
    {
        m_twistAxis = twistAxis;
    }

    void BlendTreeLookAtNode::SetLimitsEnabled(bool limitsEnabled)
    {
        m_limitsEnabled = limitsEnabled;
    }

    void BlendTreeLookAtNode::SetSmoothingEnabled(bool smoothingEnabled)
    {
        m_smoothing = smoothingEnabled;
    }

    void BlendTreeLookAtNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeLookAtNode, AnimGraphNode>()
            ->Version(2)
            ->Field("targetNodeName", &BlendTreeLookAtNode::m_targetNodeName)
            ->Field("postRotation", &BlendTreeLookAtNode::m_postRotation)
            ->Field("limitsEnabled", &BlendTreeLookAtNode::m_limitsEnabled)
            ->Field("limitMin", &BlendTreeLookAtNode::m_limitMin)
            ->Field("limitMax", &BlendTreeLookAtNode::m_limitMax)
            ->Field("constraintRotation", &BlendTreeLookAtNode::m_constraintRotation)
            ->Field("twistAxis", &BlendTreeLookAtNode::m_twistAxis)
            ->Field("smoothing", &BlendTreeLookAtNode::m_smoothing)
            ->Field("followSpeed", &BlendTreeLookAtNode::m_followSpeed);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        auto root = editContext->Class<BlendTreeLookAtNode>("Look At", "Look At attributes");
        root->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeLookAtNode::m_targetNodeName, "Node", "The node to apply the lookat to. For example the head.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeLookAtNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_postRotation, "Post rotation", "The relative rotation applied after solving.");

        root->ClassElement(AZ::Edit::ClassElements::Group, "Rotation Limits")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_limitsEnabled, "Enable limits", "Enable rotational limits?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_limitMin, "Yaw/pitch min", "The minimum rotational yaw and pitch angle limits, in degrees.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
            ->Attribute(AZ::Edit::Attributes::Min, -90.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_limitMax, "Yaw/pitch max", "The maximum rotational yaw and pitch angle limits, in degrees.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
            ->Attribute(AZ::Edit::Attributes::Min, -90.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_constraintRotation, "Constraint rotation", "A rotation that rotates the constraint space.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeLookAtNode::m_twistAxis, "Roll axis", "The axis used for twist/roll.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetLimitWidgetsVisibility);

        root->ClassElement(AZ::Edit::ClassElements::Group, "Smoothing")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_smoothing, "Enable smoothing", "Enable rotation smoothing, which is controlled by the follow speed setting.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeLookAtNode::m_followSpeed, "Follow speed", "The speed factor at which to follow the goal. A value near zero meaning super slow and a value of 1 meaning instant following.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeLookAtNode::GetFollowSpeedVisibility)
            ->Attribute(AZ::Edit::Attributes::Min, 0.05f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0);
    }
} // namespace EMotionFX
