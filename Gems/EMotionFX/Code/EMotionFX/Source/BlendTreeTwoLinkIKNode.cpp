/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendTreeTwoLinkIKNode.h"
#include "AnimGraphManager.h"
#include "EventManager.h"
#include "Node.h"
#include "Recorder.h"
#include "TransformData.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/Vector.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeTwoLinkIKNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeTwoLinkIKNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeTwoLinkIKNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeTwoLinkIKNode::UniqueData::Update()
    {
        BlendTreeTwoLinkIKNode* twoLinkIKNode = azdynamic_cast<BlendTreeTwoLinkIKNode*>(m_object);
        AZ_Assert(twoLinkIKNode, "Unique data linked to incorrect node type.");

        const ActorInstance* actorInstance = m_animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();

        // don't update the next time again
        m_nodeIndexA = InvalidIndex;
        m_nodeIndexB = InvalidIndex;
        m_nodeIndexC = InvalidIndex;
        m_alignNodeIndex = InvalidIndex;
        m_bendDirNodeIndex = InvalidIndex;
        m_endEffectorNodeIndex = InvalidIndex;
        SetHasError(true);

        // Find the end joint.
        const AZStd::string& endJointName = twoLinkIKNode->GetEndJointName();
        if (endJointName.empty())
        {
            return;
        }
        const Node* jointC = skeleton->FindNodeByName(endJointName);
        if (!jointC)
        {
            return;
        }
        m_nodeIndexC = jointC->GetNodeIndex();

        // Get the second joint.
        m_nodeIndexB = jointC->GetParentIndex();
        if (m_nodeIndexB == InvalidIndex)
        {
            return;
        }

        // Get the third joint.
        m_nodeIndexA = skeleton->GetNode(m_nodeIndexB)->GetParentIndex();
        if (m_nodeIndexA == InvalidIndex)
        {
            return;
        }

        // Get the end effector joint.
        const AZStd::string& endEffectorJointName = twoLinkIKNode->GetEndEffectorJointName();
        const Node* endEffectorJoint = skeleton->FindNodeByName(endEffectorJointName);
        if (endEffectorJoint)
        {
            m_endEffectorNodeIndex = endEffectorJoint->GetNodeIndex();
        }

        // Find the bend direction joint.
        const AZStd::string& bendDirJointName = twoLinkIKNode->GetBendDirJointName();
        const Node* bendDirJoint = skeleton->FindNodeByName(bendDirJointName);
        if (bendDirJoint)
        {
            m_bendDirNodeIndex = bendDirJoint->GetNodeIndex();
        }

        // lookup the actor instance to get the alignment node from
        const NodeAlignmentData& alignToJointData = twoLinkIKNode->GetAlignToJointData();
        const ActorInstance* alignInstance = m_animGraphInstance->FindActorInstanceFromParentDepth(alignToJointData.second);
        if (alignInstance)
        {
            if (!alignToJointData.first.empty())
            {
                const Node* alignJoint = alignInstance->GetActor()->GetSkeleton()->FindNodeByName(alignToJointData.first.c_str());
                if (alignJoint)
                {
                    m_alignNodeIndex = alignJoint->GetNodeIndex();
                }
            }
        }

        SetHasError(false);
    }

    BlendTreeTwoLinkIKNode::BlendTreeTwoLinkIKNode()
        : AnimGraphNode()
        , m_rotationEnabled(false)
        , m_relativeBendDir(true)
        , m_extractBendDir(false)
    {
        // setup the input ports
        InitInputPorts(5);
        SetupInputPort("Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsVector3("Goal Pos", INPUTPORT_GOALPOS, PORTID_INPUT_GOALPOS);
        SetupInputPortAsVector3("Bend Dir", INPUTPORT_BENDDIR, PORTID_INPUT_BENDDIR);
        SetupInputPort("Goal Rot", INPUTPORT_GOALROT, MCore::AttributeQuaternion::TYPE_ID, PORTID_INPUT_GOALROT);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    BlendTreeTwoLinkIKNode::~BlendTreeTwoLinkIKNode()
    {
    }

    bool BlendTreeTwoLinkIKNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeTwoLinkIKNode::GetPaletteName() const
    {
        return "TwoLink IK";
    }

    // get the category
    AnimGraphObject::ECategory BlendTreeTwoLinkIKNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }

    // solve the IK problem by calculating the 'knee/elbow' position
    bool BlendTreeTwoLinkIKNode::Solve2LinkIK(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Vector3* outMidPos)
    {
        const AZ::Vector3 localGoal = goal - posA;
        const float rLen = MCore::SafeLength(localGoal);

        // get the lengths of the bones A and B
        const float lengthA = MCore::SafeLength(posB - posA);
        const float lengthB = MCore::SafeLength(posC - posB);

        // calculate the d and e values from the equations by Ken Perlin
        const float d = (rLen > MCore::Math::epsilon) ? MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, (rLen + (lengthA * lengthA - lengthB * lengthB) / rLen) * 0.5f)) : MCore::Max<float>(0.0f, MCore::Min<float>(lengthA, rLen));
        const float square = lengthA * lengthA - d * d;
        const float e = MCore::Math::SafeSqrt(square);

        // the solution on the YZ plane
        const AZ::Vector3 solution(d, e, 0);

        // calculate the matrix that rotates from IK solve space into world space
        AZ::Matrix3x3 matForward = AZ::Matrix3x3::CreateIdentity();
        CalculateMatrix(localGoal, bendDir, &matForward);

        // rotate the solution (the mid "knee/elbow" position) into world space
        *outMidPos = posA + solution * matForward;

        // check if we found a solution or not
        return (d > MCore::Math::epsilon && d < lengthA + MCore::Math::epsilon);
    }

    // calculate the direction matrix
    void BlendTreeTwoLinkIKNode::CalculateMatrix(const AZ::Vector3& goal, const AZ::Vector3& bendDir, AZ::Matrix3x3* outForward)
    {
        // the inverse matrix defines a coordinate system whose x axis contains P, so X = unit(P).
        const AZ::Vector3 x = goal.GetNormalizedSafe(); 

        // the y axis of the inverse is perpendicular to P, so Y = unit( D - X(D . X) ).
        const float dot = bendDir.Dot(x);
        const AZ::Vector3 y = (bendDir - (dot * x)).GetNormalizedSafe();

        // the z axis of the inverse is perpendicular to both X and Y, so Z = X x Y.
        const AZ::Vector3 z = x.Cross(y);

        // set the rotation vectors of the output matrix
        outForward->SetRow(0, x);
        outForward->SetRow(1, y);
        outForward->SetRow(2, z);
    }

    // the main process method of the final node
    void BlendTreeTwoLinkIKNode::Output(AnimGraphInstance* animGraphInstance)
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

        // if the IK weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || m_disabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        // get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        //------------------------------------
        // get the node indices to work on
        //------------------------------------
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (uniqueData->GetHasError())
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, true);
            }
            return;
        }

        // get the node indices
        const size_t nodeIndexA = uniqueData->m_nodeIndexA;
        const size_t nodeIndexB = uniqueData->m_nodeIndexB;
        const size_t nodeIndexC = uniqueData->m_nodeIndexC;
        const size_t bendDirIndex = uniqueData->m_bendDirNodeIndex;
        size_t alignNodeIndex = uniqueData->m_alignNodeIndex;
        size_t endEffectorNodeIndex = uniqueData->m_endEffectorNodeIndex;

        // use the end node as end effector node if no goal node has been specified
        if (endEffectorNodeIndex == InvalidIndex)
        {
            endEffectorNodeIndex = nodeIndexC;
        }

        // get the goal
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALPOS));
        AZ::Vector3 goal = AZ::Vector3::CreateZero();
        TryGetInputVector3(animGraphInstance, INPUTPORT_GOALPOS, goal);

        // there is no error, as we have all we need to solve this
        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, false);
        }

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        ActorInstance* alignInstance = actorInstance;
        EMotionFX::Transform alignNodeTransform;

        // adjust the gizmo offset value
        if (alignNodeIndex != InvalidIndex)
        {
            // update the alignment actor instance
            alignInstance = animGraphInstance->FindActorInstanceFromParentDepth(m_alignToNode.second);
            if (alignInstance)
            {
                if (m_alignToNode.second == 0)
                { // we are aligning to a node in our current graphInstance so we can use the input pose
                    alignNodeTransform = inputPose->GetPose().GetWorldSpaceTransform(alignNodeIndex);
                }
                else
                {
                    alignNodeTransform = alignInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(alignNodeIndex);
                }
                const AZ::Vector3& offset = alignNodeTransform.m_position;
                goal += offset;

                if (GetEMotionFX().GetIsInEditorMode())
                {
                    // check if the offset goal pos values comes from a param node
                    const BlendTreeConnection* posConnection = GetInputPort(INPUTPORT_GOALPOS).m_connection;
                    if (posConnection)
                    {
                        if (azrtti_typeid(posConnection->GetSourceNode()) == azrtti_typeid<BlendTreeParameterNode>())
                        {
                            BlendTreeParameterNode* parameterNode = static_cast<BlendTreeParameterNode*>(posConnection->GetSourceNode());
                            GetEventManager().OnSetVisualManipulatorOffset(animGraphInstance, parameterNode->GetParameterIndex(posConnection->GetSourcePort()), offset);
                        }
                    }
                }
            }
            else
            {
                alignNodeIndex = InvalidIndex; // we were not able to get the align instance, so set the align node index to the invalid index
            }
        }
        else if (GetEMotionFX().GetIsInEditorMode())
        {
            const BlendTreeConnection* posConnection = GetInputPort(INPUTPORT_GOALPOS).m_connection;
            if (posConnection)
            {
                if (azrtti_typeid(posConnection->GetSourceNode()) == azrtti_typeid<BlendTreeParameterNode>())
                {
                    BlendTreeParameterNode* parameterNode = static_cast<BlendTreeParameterNode*>(posConnection->GetSourceNode());
                    GetEventManager().OnSetVisualManipulatorOffset(animGraphInstance, parameterNode->GetParameterIndex(posConnection->GetSourcePort()), AZ::Vector3::CreateZero());
                }
            }
        }

        //------------------------------------
        // perform the main calculation part
        //------------------------------------
        Pose& outTransformPose = outputPose->GetPose();
        Transform globalTransformA = outTransformPose.GetWorldSpaceTransform(nodeIndexA);
        Transform globalTransformB = outTransformPose.GetWorldSpaceTransform(nodeIndexB);
        Transform globalTransformC = outTransformPose.GetWorldSpaceTransform(nodeIndexC);

        // extract the bend direction from the input pose?
        AZ::Vector3 bendDir;
        if (m_extractBendDir)
        {
            if (bendDirIndex != InvalidIndex)
            {
                bendDir = outTransformPose.GetWorldSpaceTransform(bendDirIndex).m_position - globalTransformA.m_position;
            }
            else
            {
                bendDir = globalTransformB.m_position - globalTransformA.m_position;
            }
        }
        else
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_BENDDIR));
            if (!TryGetInputVector3(animGraphInstance, INPUTPORT_BENDDIR, bendDir))
            {
                bendDir = AZ::Vector3(0.0f, 0.0f, -1.0f);
            }
        }

        // if we want a relative bend dir, rotate it with the actor (only do this if we don't extract the bend dir)
        if (m_relativeBendDir && !m_extractBendDir)
        {
            bendDir = actorInstance->GetWorldSpaceTransform().m_rotation.TransformVector(bendDir);
            bendDir.NormalizeSafe();
        }
        else
        {
            bendDir.NormalizeSafe();
        }

        // if end node rotation is enabled
        if (m_rotationEnabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALROT));
            const MCore::AttributeQuaternion* inputGoalRot = GetInputQuaternion(animGraphInstance, INPUTPORT_GOALROT);

            // if we don't want to align the rotation and position to another given node
            if (alignNodeIndex == InvalidIndex)
            {
                AZ::Quaternion newRotation = AZ::Quaternion::CreateIdentity(); // identity quat
                if (inputGoalRot)
                {
                    newRotation = inputGoalRot->GetValue(); // use our new rotation directly
                }
                globalTransformC.m_rotation = newRotation;
                outTransformPose.SetWorldSpaceTransform(nodeIndexC, globalTransformC);
            }
            else // align to another node
            {
                if (inputGoalRot)
                {
                    OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALROT));
                    globalTransformC.m_rotation = GetInputQuaternion(animGraphInstance, INPUTPORT_GOALROT)->GetValue() * alignNodeTransform.m_rotation;
                }
                else
                {
                    globalTransformC.m_rotation = alignNodeTransform.m_rotation;
                }

                outTransformPose.SetWorldSpaceTransform(nodeIndexC, globalTransformC);
            }
        }

        // adjust the goal and get the end effector position
        AZ::Vector3 endEffectorNodePos = outTransformPose.GetWorldSpaceTransform(endEffectorNodeIndex).m_position;
        const AZ::Vector3 posCToEndEffector = endEffectorNodePos - globalTransformC.m_position;
        if (m_rotationEnabled)
        {
            goal -= posCToEndEffector;
        }

        // store the desired rotation
        AZ::Quaternion newNodeRotationC = globalTransformC.m_rotation;

        // draw debug lines
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            const float s = animGraphInstance->GetVisualizeScale() * actorInstance->GetVisualizeScale();

            AZ::Vector3 realGoal;
            if (m_rotationEnabled)
            {
                realGoal = goal + posCToEndEffector;
            }
            else
            {
                realGoal = goal;
            }

            DebugDraw& debugDraw = GetDebugDraw();
            DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(animGraphInstance->GetActorInstance());
            drawData->Lock();
            drawData->DrawLine(realGoal - AZ::Vector3(s, 0, 0), realGoal + AZ::Vector3(s, 0, 0), m_visualizeColor);
            drawData->DrawLine(realGoal - AZ::Vector3(0, s, 0), realGoal + AZ::Vector3(0, s, 0), m_visualizeColor);
            drawData->DrawLine(realGoal - AZ::Vector3(0, 0, s), realGoal + AZ::Vector3(0, 0, s), m_visualizeColor);

            const AZ::Color color(0.0f, 1.0f, 1.0f, 1.0f);
            drawData->DrawLine(globalTransformA.m_position, globalTransformA.m_position + bendDir * s * 2.5f, color);
            drawData->DrawLine(globalTransformA.m_position - AZ::Vector3(s, 0, 0), globalTransformA.m_position + AZ::Vector3(s, 0, 0), color);
            drawData->DrawLine(globalTransformA.m_position - AZ::Vector3(0, s, 0), globalTransformA.m_position + AZ::Vector3(0, s, 0), color);
            drawData->DrawLine(globalTransformA.m_position - AZ::Vector3(0, 0, s), globalTransformA.m_position + AZ::Vector3(0, 0, s), color);
            drawData->Unlock();
        }

        // perform IK, try to find a solution by calculating the new middle node position
        AZ::Vector3 midPos;
        if (m_rotationEnabled)
        {
            Solve2LinkIK(globalTransformA.m_position, globalTransformB.m_position, globalTransformC.m_position, goal, bendDir, &midPos);
        }
        else
        {
            Solve2LinkIK(globalTransformA.m_position, globalTransformB.m_position, endEffectorNodePos, goal, bendDir, &midPos);
        }

        // --------------------------------------
        // calculate the new node transforms
        // --------------------------------------
        // calculate the differences between the current forward vector and the new one after IK
        AZ::Vector3 oldForward = globalTransformB.m_position - globalTransformA.m_position;
        AZ::Vector3 newForward = midPos - globalTransformA.m_position;
        oldForward.NormalizeSafe();
        newForward.NormalizeSafe();

        // perform a delta rotation to rotate into the new direction after IK
        float dotProduct = oldForward.Dot(newForward);
        float deltaAngle = MCore::Math::ACos(MCore::Clamp<float>(dotProduct, -1.0f, 1.0f));
        AZ::Vector3 axis = oldForward.Cross(newForward);
        if (axis.GetLengthSq() > 0.0f)
        {
            globalTransformA.m_rotation = AZ::Quaternion::CreateFromAxisAngle(
                axis.GetNormalized(), deltaAngle) * globalTransformA.m_rotation;
        }
        outTransformPose.SetWorldSpaceTransform(nodeIndexA, globalTransformA);

        // globalTransformA = outTransformPose.GetGlobalTransformIncludingActorInstanceTransform(nodeIndexA);
        globalTransformB = outTransformPose.GetWorldSpaceTransform(nodeIndexB);
        globalTransformC = outTransformPose.GetWorldSpaceTransform(nodeIndexC);

        // get the new current node positions
        midPos = globalTransformB.m_position;
        endEffectorNodePos = outTransformPose.GetWorldSpaceTransform(endEffectorNodeIndex).m_position;

        // second node
        if (m_rotationEnabled)
        {
            oldForward = globalTransformC.m_position - globalTransformB.m_position;
        }
        else
        {
            oldForward = endEffectorNodePos - globalTransformB.m_position;
        }

        oldForward.NormalizeSafe();
        newForward = (goal - globalTransformB.m_position).GetNormalizedSafe();

        // calculate the delta rotation
        dotProduct = oldForward.Dot(newForward);
        if (dotProduct < 1.0f - MCore::Math::epsilon)
        {
            deltaAngle = MCore::Math::ACos(MCore::Clamp<float>(dotProduct, -1.0f, 1.0f));
            axis = oldForward.Cross(newForward);
            if (axis.GetLengthSq() > 0.0f)
            {
                globalTransformB.m_rotation = AZ::Quaternion::CreateFromAxisAngle(
                    axis.GetNormalized(), deltaAngle) * globalTransformB.m_rotation;
            }
        }

        globalTransformB.m_position = midPos;
        outTransformPose.SetWorldSpaceTransform(nodeIndexB, globalTransformB);

        // update the rotation of node C
        if (m_rotationEnabled)
        {
            globalTransformC = outTransformPose.GetWorldSpaceTransform(nodeIndexC);
            globalTransformC.m_rotation = newNodeRotationC;
            outTransformPose.SetWorldSpaceTransform(nodeIndexC, globalTransformC);
        }

        // only blend when needed
        if (weight < 0.999f)
        {
            // get the original input pose
            const Pose& inputTransformPose = inputPose->GetPose();

            // get the original input transforms
            Transform finalTransformA = inputTransformPose.GetLocalSpaceTransform(nodeIndexA);
            Transform finalTransformB = inputTransformPose.GetLocalSpaceTransform(nodeIndexB);
            Transform finalTransformC = inputTransformPose.GetLocalSpaceTransform(nodeIndexC);

            // blend them into the new transforms after IK
            finalTransformA.Blend(outTransformPose.GetLocalSpaceTransform(nodeIndexA), weight);
            finalTransformB.Blend(outTransformPose.GetLocalSpaceTransform(nodeIndexB), weight);
            finalTransformC.Blend(outTransformPose.GetLocalSpaceTransform(nodeIndexC), weight);

            // copy them to the output transforms
            outTransformPose.SetLocalSpaceTransform(nodeIndexA, finalTransformA);
            outTransformPose.SetLocalSpaceTransform(nodeIndexB, finalTransformB);
            outTransformPose.SetLocalSpaceTransform(nodeIndexC, finalTransformC);
        }

        // render some debug lines
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            DebugDraw& debugDraw = GetDebugDraw();
            DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(animGraphInstance->GetActorInstance());
            drawData->Lock();
            drawData->DrawLine(outTransformPose.GetWorldSpaceTransform(nodeIndexA).m_position, outTransformPose.GetWorldSpaceTransform(nodeIndexB).m_position, m_visualizeColor);
            drawData->DrawLine(outTransformPose.GetWorldSpaceTransform(nodeIndexB).m_position, outTransformPose.GetWorldSpaceTransform(nodeIndexC).m_position, m_visualizeColor);
            drawData->Unlock();
        }
    }

    AZ::Crc32 BlendTreeTwoLinkIKNode::GetRelativeBendDirVisibility() const
    {
        return m_extractBendDir ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    void BlendTreeTwoLinkIKNode::SetRelativeBendDir(bool relativeBendDir)
    {
        m_relativeBendDir = relativeBendDir;
    }

    void BlendTreeTwoLinkIKNode::SetExtractBendDir(bool extractBendDir)
    {
        m_extractBendDir = extractBendDir;
    }

    void BlendTreeTwoLinkIKNode::SetRotationEnabled(bool rotationEnabled)
    {
        m_rotationEnabled = rotationEnabled;
    }

    void BlendTreeTwoLinkIKNode::SetBendDirNodeName(const AZStd::string& bendDirNodeName)
    {
        m_bendDirNodeName = bendDirNodeName;
    }

    void BlendTreeTwoLinkIKNode::SetAlignToNode(const NodeAlignmentData& alignToNode)
    {
        m_alignToNode = alignToNode;
    }

    void BlendTreeTwoLinkIKNode::SetEndEffectorNodeName(const AZStd::string& endEffectorNodeName)
    {
        m_endEffectorNodeName = endEffectorNodeName;
    }

    void BlendTreeTwoLinkIKNode::SetEndNodeName(const AZStd::string& endNodeName)
    {
        m_endNodeName = endNodeName;
    }

    void BlendTreeTwoLinkIKNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeTwoLinkIKNode, AnimGraphNode>()
            ->Field("endNodeName", &BlendTreeTwoLinkIKNode::m_endNodeName)
            ->Field("endEffectorNodeName", &BlendTreeTwoLinkIKNode::m_endEffectorNodeName)
            ->Field("alignToNode", &BlendTreeTwoLinkIKNode::m_alignToNode)
            ->Field("bendDirNodeName", &BlendTreeTwoLinkIKNode::m_bendDirNodeName)
            ->Field("rotationEnabled", &BlendTreeTwoLinkIKNode::m_rotationEnabled)
            ->Field("relativeBendDir", &BlendTreeTwoLinkIKNode::m_relativeBendDir)
            ->Field("extractBendDir", &BlendTreeTwoLinkIKNode::m_extractBendDir)
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeTwoLinkIKNode>("Two Link IK", "Two Link IK attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeTwoLinkIKNode::m_endNodeName, "End Node", "The end node name of the chain, for example the foot, or hand.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeTwoLinkIKNode::m_endEffectorNodeName, "End Effector", "The end effector node, which represents the node that actually tries to reach the goal. This is probably also the hand, or a child node of it for example. If not set, the end node is used.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorGoalNode"), &BlendTreeTwoLinkIKNode::m_alignToNode, "Align To", "The node to align the end node to. This basically sets the goal to this node.")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeTwoLinkIKNode::m_bendDirNodeName, "Bend Dir Node", "The optional node to control the bend direction. The vector from the start node to the bend dir node will be used as bend direction.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeTwoLinkIKNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTwoLinkIKNode::m_rotationEnabled, "Enable Rotation Goal", "Enable the goal orientation?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTwoLinkIKNode::m_relativeBendDir, "Relative Bend Dir", "Use a relative (to the actor instance) bend direction, instead of world space?")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeTwoLinkIKNode::GetRelativeBendDirVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeTwoLinkIKNode::m_extractBendDir, "Extract Bend Dir", "Extract the bend direction from the input pose instead of using the bend dir input value?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree);
    }
} // namespace EMotionFX
