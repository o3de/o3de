/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendTreeFabrikNode.h"
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
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFabrikNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFabrikNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeFabrikNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeFabrikNode::UniqueData::Update()
    {
        BlendTreeFabrikNode* fabrikIKNode = azdynamic_cast<BlendTreeFabrikNode*>(m_object);
        AZ_Assert(fabrikIKNode, "Unique data linked to incorrect node type.");

        const ActorInstance* actorInstance = m_animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();

        // don't update the next time again
        m_nodeIndices.clear();
        m_alignNodeIndex = InvalidIndex;
        m_bendDirNodeIndex = InvalidIndex;
        m_endEffectorNodeIndex = InvalidIndex;
        SetHasError(true);

        const AZStd::string& rootJointName = fabrikIKNode->GetRootJointName();
        // Find the end joint.
        const AZStd::string& endJointName = fabrikIKNode->GetEndJointName();
        if (rootJointName.empty() || endJointName.empty() || rootJointName == endJointName)
        {
            return;
        }
        const Node* rootJoint = skeleton->FindNodeByName(rootJointName);
        if (!rootJoint)
        {
            return;
        }

        for (Node* joint = skeleton->FindNodeByName(endJointName); joint != rootJoint; joint = joint->GetParentNode())
        {
            if (joint == nullptr)
            {
                // it means the root node is not the ancester of end joint
                return;
            }
            m_nodeIndices.push_back(joint->GetNodeIndex());
        }
        m_nodeIndices.push_back(rootJoint->GetNodeIndex());
        AZStd::reverse(m_nodeIndices.begin(), m_nodeIndices.end());

        // Get the end effector joint.
        const AZStd::string& endEffectorJointName = fabrikIKNode->GetEndEffectorJointName();
        const Node* endEffectorJoint = skeleton->FindNodeByName(endEffectorJointName);
        if (endEffectorJoint)
        {
            m_endEffectorNodeIndex = endEffectorJoint->GetNodeIndex();
        }

        // Find the bend direction joint.
        const AZStd::string& bendDirJointName = fabrikIKNode->GetBendDirJointName();
        const Node* bendDirJoint = skeleton->FindNodeByName(bendDirJointName);
        if (bendDirJoint)
        {
            m_bendDirNodeIndex = bendDirJoint->GetNodeIndex();
        }

        // lookup the actor instance to get the alignment node from
        const NodeAlignmentData& alignToJointData = fabrikIKNode->GetAlignToJointData();
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

    BlendTreeFabrikNode::BlendTreeFabrikNode()
        : AnimGraphNode()
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

    BlendTreeFabrikNode::~BlendTreeFabrikNode()
    {
    }

    bool BlendTreeFabrikNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeFabrikNode::GetPaletteName() const
    {
        return "FABRIK";
    }

    // get the category
    AnimGraphObject::ECategory BlendTreeFabrikNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }

    bool BlendTreeFabrikNode::SolveFabrik(const AZ::Vector3& goal, AZStd::vector<AZ::Vector3>& positions, const AZ::Vector3& bendDir, bool hasBendDir, int iterations, float delta)
    {
        // initial position of the root bone
        AZ::Vector3 rootPosition = positions[0];
        const AZ::Vector3 localGoal = goal - rootPosition;
        const float rLen = MCore::SafeLength(localGoal);
        size_t boneSize = positions.size();

        // do nothing if there is only one joint.
        if (boneSize < 1)
        {
            return false;
        }
        AZStd::vector<float> boneLengths;

        // get the lengths of the bones
        float totalBoneLength = 0.0f;
        for (int i = 0; i < boneSize - 1; i++)
        {
            boneLengths.push_back(MCore::SafeLength(positions[i + 1] - positions[i]));
            totalBoneLength += boneLengths[i];
        }

        if (totalBoneLength < rLen)
        {
            // set every bone's rotation to the direction from root joint to goal position.
            AZ::Vector3 direction = localGoal.GetNormalizedSafe();
            for (int i = 1; i < boneSize; i++)
            {
                positions[i] = positions[i - 1] + direction * boneLengths[i - 1];
            }
            return false;
        }

        for (; iterations > 0; iterations--)
        {
            //https://www.youtube.com/watch?v=UNoX65PRehA
            // backward
            positions[boneSize - 1] = goal;
            for (size_t i = boneSize - 2; i > 0; i--)
            {
                positions[i] = positions[i + 1] + (positions[i] - positions[i + 1]).GetNormalizedSafe() * boneLengths[i];
            }
            // forward
            positions[0] = rootPosition;
            for (size_t i = 1; i < boneSize; i++)
            {
                positions[i] = positions[i - 1] + (positions[i] - positions[i - 1]).GetNormalizedSafe() * boneLengths[i - 1];
            }
            //close enough?
            if (MCore::SafeLength(positions[boneSize - 1] - goal) < delta)
            {
                break;
            }
        }

        if (!hasBendDir)
        {
            return true;
        }

        for (int i = 1; i < boneSize - 1; i++)
        {
            // bend the joint towards bendDir with axis of the adjacent joints on the chain
            AZ::Vector3 axis = (positions[i + 1] - positions[i - 1]).GetNormalizedSafe();
            AZ::Vector3 currentDir = positions[i] - positions[i - 1];
            AZ::Vector3 v1 = currentDir - axis * (axis.Dot(currentDir));
            AZ::Vector3 v2 = bendDir - axis * (axis.Dot(bendDir));
            AZ::Quaternion bendRotation = AZ::Quaternion::CreateShortestArc(v1, v2);
            positions[i] = bendRotation.TransformVector(currentDir) + positions[i - 1];
        }
        return true;
    }

    // the main process method of the final node
    void BlendTreeFabrikNode::Output(AnimGraphInstance* animGraphInstance)
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

        // get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;
        // if the IK weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || m_disabled)
        {
            return;
        }

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
        const AZStd::vector<size_t>& nodeIndices = uniqueData->m_nodeIndices;
        const size_t bendDirIndex = uniqueData->m_bendDirNodeIndex;
        size_t alignNodeIndex = uniqueData->m_alignNodeIndex;
        size_t endEffectorNodeIndex = uniqueData->m_endEffectorNodeIndex;
        size_t endNodeIndex = nodeIndices.back();

        // use the end node as end effector node if no goal node has been specified
        if (endEffectorNodeIndex == InvalidIndex)
        {
            endEffectorNodeIndex = endNodeIndex;
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
        AZStd::vector<Transform> transforms;
        transforms.resize(nodeIndices.size());
        for (int i = 0; i < nodeIndices.size(); i++)
        {
            transforms[i] = outTransformPose.GetWorldSpaceTransform(nodeIndices[i]);
        }

        Transform& endNodeTransform = transforms.back();

        // extract the bend direction from the input pose?
        AZ::Vector3 bendDir;
        bool hasBendDir = true;
        if (m_extractBendDir)
        {
            if (bendDirIndex != InvalidIndex)
            {
                bendDir = outTransformPose.GetWorldSpaceTransform(bendDirIndex).m_position - transforms[0].m_position;
            }
            else
            {
                hasBendDir = false;
            }
        }
        else
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_BENDDIR));
            if (!TryGetInputVector3(animGraphInstance, INPUTPORT_BENDDIR, bendDir))
            {
                hasBendDir = false;
            }
        }

        // if we want a relative bend dir, rotate it with the actor (only do this if we don't extract the bend dir)
        if (m_relativeBendDir && !m_extractBendDir)
        {
            bendDir = actorInstance->GetWorldSpaceTransform().m_rotation.TransformVector(bendDir);
        }

        bendDir.NormalizeSafe();

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
                endNodeTransform.m_rotation = newRotation;
                outTransformPose.SetWorldSpaceTransform(endNodeIndex, endNodeTransform);
            }
            else // align to another node
            {
                if (inputGoalRot)
                {
                    OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALROT));
                    endNodeTransform.m_rotation = GetInputQuaternion(animGraphInstance, INPUTPORT_GOALROT)->GetValue() * alignNodeTransform.m_rotation;
                }
                else
                {
                    endNodeTransform.m_rotation = alignNodeTransform.m_rotation;
                }

                outTransformPose.SetWorldSpaceTransform(endNodeIndex, endNodeTransform);
            }
        }

        // adjust the goal and get the end effector position
        AZ::Vector3 endEffectorNodePos = outTransformPose.GetWorldSpaceTransform(endEffectorNodeIndex).m_position;
        const AZ::Vector3 posCToEndEffector = endEffectorNodePos - endNodeTransform.m_position;
        if (m_rotationEnabled)
        {
            goal -= posCToEndEffector;
        }

        // store the desired rotation
        AZ::Quaternion newNodeRotationC = endNodeTransform.m_rotation;

        // draw debug lines
        AZ::Vector3 rootPosition = transforms[0].m_position;
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
            
            drawData->DrawLine(rootPosition, rootPosition + bendDir * s * 2.5f, color);
            drawData->DrawLine(rootPosition - AZ::Vector3(s, 0, 0), rootPosition + AZ::Vector3(s, 0, 0), color);
            drawData->DrawLine(rootPosition - AZ::Vector3(0, s, 0), rootPosition + AZ::Vector3(0, s, 0), color);
            drawData->DrawLine(rootPosition - AZ::Vector3(0, 0, s), rootPosition + AZ::Vector3(0, 0, s), color);
            drawData->Unlock();
        }

        // perform IK
        AZStd::vector<AZ::Vector3> positions;
        positions.resize(transforms.size());
        for (int i = 0; i < transforms.size(); i++)
        {
            positions[i] = transforms[i].m_position;
        }
        if (m_rotationEnabled)
        {
            positions.back() = endEffectorNodePos;
        }
        SolveFabrik(goal, positions, bendDir, hasBendDir, m_iterations, m_precision);
        // --------------------------------------
        // calculate the new node transforms
        // --------------------------------------
        // calculate the differences between the current forward vector and the new one after IK
        for (int i = 0; i < positions.size() - 1; i++)
        {
            AZ::Vector3 oldForward = transforms[i + 1].m_position - transforms[i].m_position;
            AZ::Vector3 newForward = positions[i + 1] - positions[i];
            oldForward.NormalizeSafe();
            newForward.NormalizeSafe();
            AZ::Quaternion deltaRotation = AZ::Quaternion::CreateShortestArc(oldForward, newForward);

            // perform a delta rotation to rotate into the new direction after IK
            Transform newTransform = transforms[i];
            newTransform.m_rotation = deltaRotation * transforms[i].m_rotation;
            newTransform.m_position = positions[i];

            outTransformPose.SetWorldSpaceTransform(nodeIndices[i], newTransform);
        }
        
        Transform globalTransformC = endNodeTransform;
        globalTransformC.m_position = positions.back();
        if (m_rotationEnabled)
        {
            // update the rotation of end node
            globalTransformC.m_rotation = newNodeRotationC;
        }
        outTransformPose.SetWorldSpaceTransform(endNodeIndex, globalTransformC);
        // only blend when needed
        if (weight < 0.999f)
        {
            // get the original input pose
            const Pose& inputTransformPose = inputPose->GetPose();

            for (int i = 0; i < nodeIndices.size(); i++)
            {
                // get the original input transforms
                Transform finalTransform = inputTransformPose.GetLocalSpaceTransform(nodeIndices[i]);
                // blend them into the new transforms after IK
                finalTransform.Blend(outTransformPose.GetLocalSpaceTransform(nodeIndices[i]), weight);
                // copy them to the output transforms
                outTransformPose.SetLocalSpaceTransform(nodeIndices[i], finalTransform);
            }
        }

        // render some debug lines
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            DebugDraw& debugDraw = GetDebugDraw();
            DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(animGraphInstance->GetActorInstance());
            drawData->Lock();
            for (int i = 0; i < nodeIndices.size() - 1; i++)
            {
                drawData->DrawLine(outTransformPose.GetWorldSpaceTransform(nodeIndices[i]).m_position, 
                outTransformPose.GetWorldSpaceTransform(nodeIndices[i + 1]).m_position, m_visualizeColor);
            }
            drawData->Unlock();
        }
    }

    AZ::Crc32 BlendTreeFabrikNode::GetRelativeBendDirVisibility() const
    {
        return m_extractBendDir ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    void BlendTreeFabrikNode::SetRelativeBendDir(bool relativeBendDir)
    {
        m_relativeBendDir = relativeBendDir;
    }

    void BlendTreeFabrikNode::SetExtractBendDir(bool extractBendDir)
    {
        m_extractBendDir = extractBendDir;
    }

    void BlendTreeFabrikNode::SetRotationEnabled(bool rotationEnabled)
    {
        m_rotationEnabled = rotationEnabled;
    }

    void BlendTreeFabrikNode::SetBendDirNodeName(const AZStd::string& bendDirNodeName)
    {
        m_bendDirNodeName = bendDirNodeName;
    }

    void BlendTreeFabrikNode::SetAlignToNode(const NodeAlignmentData& alignToNode)
    {
        m_alignToNode = alignToNode;
    }

    void BlendTreeFabrikNode::SetEndEffectorNodeName(const AZStd::string& endEffectorNodeName)
    {
        m_endEffectorNodeName = endEffectorNodeName;
    }

    void BlendTreeFabrikNode::SetEndNodeName(const AZStd::string& endNodeName)
    {
        m_endNodeName = endNodeName;
    }

    void BlendTreeFabrikNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFabrikNode, AnimGraphNode>()
            ->Field("rootNodeName", &BlendTreeFabrikNode::m_rootNodeName)
            ->Field("endNodeName", &BlendTreeFabrikNode::m_endNodeName)
            ->Field("endEffectorNodeName", &BlendTreeFabrikNode::m_endEffectorNodeName)
            ->Field("alignToNode", &BlendTreeFabrikNode::m_alignToNode)
            ->Field("bendDirNodeName", &BlendTreeFabrikNode::m_bendDirNodeName)
            ->Field("rotationEnabled", &BlendTreeFabrikNode::m_rotationEnabled)
            ->Field("relativeBendDir", &BlendTreeFabrikNode::m_relativeBendDir)
            ->Field("extractBendDir", &BlendTreeFabrikNode::m_extractBendDir)
            ->Field("iterations", &BlendTreeFabrikNode::m_iterations)
            ->Field("precision", &BlendTreeFabrikNode::m_precision)
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFabrikNode>("FABRIK", "FABRIK attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeFabrikNode::m_rootNodeName, "Root Node", "The root node name of the chain.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFabrikNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeFabrikNode::m_endNodeName, "End Node", "The end node name of the chain, for example the foot, or hand.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFabrikNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeFabrikNode::m_endEffectorNodeName, "End Effector", "The end effector node, which represents the node that actually tries to reach the goal. This is probably also the hand, or a child node of it for example. If not set, the end node is used.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFabrikNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorGoalNode"), &BlendTreeFabrikNode::m_alignToNode, "Align To", "The node to align the end node to. This basically sets the goal to this node.")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFabrikNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("ActorNode"), &BlendTreeFabrikNode::m_bendDirNodeName, "Bend Dir Node", "The optional node to control the bend direction. The vector from the start node to the bend dir node will be used as bend direction.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFabrikNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFabrikNode::m_rotationEnabled, "Enable Rotation Goal", "Enable the goal orientation?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFabrikNode::m_relativeBendDir, "Relative Bend Dir", "Use a relative (to the actor instance) bend direction, instead of world space?")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendTreeFabrikNode::GetRelativeBendDirVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFabrikNode::m_extractBendDir, "Extract Bend Dir", "Extract the bend direction from the input pose instead of using the bend dir input value?")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFabrikNode::m_iterations, "Iterations", "Iterations per update for the solver")
            ->Attribute(AZ::Edit::Attributes::Min, 5)
            ->Attribute(AZ::Edit::Attributes::Max, 20)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFabrikNode::m_precision, "Precision", "Distance when the solver stops")
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
            ;
    }
} // namespace EMotionFX
