/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/RagdollInstance.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(RagdollInstance, EMotionFX::ActorAllocator)

    RagdollInstance::RagdollInstance()
        : m_lastPos(AZ::Vector3::CreateZero())
        , m_lastRot(AZ::Quaternion::CreateIdentity())
        , m_currentPos(AZ::Vector3::CreateZero())
        , m_currentRot(AZ::Quaternion::CreateIdentity())
        , m_trajectoryDeltaPos(AZ::Vector3::CreateZero())
        , m_trajectoryDeltaRot(AZ::Quaternion::CreateIdentity())
        , m_actorInstance(nullptr)
        , m_ragdollRootJoint(nullptr)
        , m_ragdollUsedLastFrame(false)
        , m_ragdollUsedThisFrame(false)
    {
        m_velocityEvaluator = AZStd::unique_ptr<RagdollVelocityEvaluator>(aznew RunningAverageVelocityEvaluator());
    }

    RagdollInstance::RagdollInstance(Physics::Ragdoll* ragdoll, ActorInstance* actorInstance)
        : RagdollInstance()
    {
        m_ragdoll = ragdoll;
        m_actorInstance = actorInstance;

        if (m_actorInstance)
        {
            const Actor* actor = m_actorInstance->GetActor();
            const Skeleton* skeleton = actor->GetSkeleton();
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            const size_t jointCount = skeleton->GetNumNodes();
            const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
            const size_t ragdollNodeCount = ragdollConfig.m_nodes.size();

            m_ragdollNodeIndices.resize(jointCount);
            m_jointIndicesByRagdollNodeIndices.resize(ragdollNodeCount);

            for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
            {
                const Node* joint = skeleton->GetNode(jointIndex);

                const AZ::Outcome<size_t> ragdollNodeIndex = ragdollConfig.FindNodeConfigIndexByName(joint->GetNameString());
                if (ragdollNodeIndex.IsSuccess())
                {
                    // Animation skeleton to ragdoll node index mapping.
                    m_ragdollNodeIndices[jointIndex] = ragdollNodeIndex.GetValue();

                    // Ragdoll node index to animation skeleton joint index mapping.
                    m_jointIndicesByRagdollNodeIndices[ragdollNodeIndex.GetValue()] = jointIndex;
                }
                else
                {
                    m_ragdollNodeIndices[jointIndex] = InvalidIndex;
                }
            }

            // Find and store the ragdoll root joint by iterating the skeleton top-down until we find the first node which is part of the ragdoll.
            for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
            {
                Node* joint = skeleton->GetNode(jointIndex);
                const AZ::Outcome<size_t> ragdollNodeIndex = GetRagdollNodeIndex(jointIndex);
                if (ragdollNodeIndex.IsSuccess())
                {
                    m_ragdollRootJoint = joint;
                    break;
                }
            }

            // Initialize the ragdoll states and transformation data.
            if (m_ragdoll)
            {
                ReadRagdollState(m_lastState, m_lastPos, m_lastRot);

                m_currentState = m_lastState;
                m_currentPos = m_lastPos;
                m_currentRot = m_lastRot;

                m_trajectoryDeltaPos = AZ::Vector3::CreateZero();
                m_trajectoryDeltaRot = AZ::Quaternion::CreateIdentity();
            }
        }
        else
        {
            m_ragdollNodeIndices.clear();
            m_jointIndicesByRagdollNodeIndices.clear();
            m_ragdollRootJoint = nullptr;
        }
    }

    void RagdollInstance::PostPhysicsUpdate(float timeDelta)
    {
        AZ_UNUSED(timeDelta);
        if (!m_ragdoll)
        {
            return;
        }

        // Step made, the current pose is now the last pose.
        m_lastState = m_currentState;
        m_lastPos = m_currentPos;
        m_lastRot = m_currentRot;

        // Read the current transforms and other data from the ragdoll in case it is already active.
        // In case the ragdoll is not active yet, the transforms will be extracted from the actor instance.
        ReadRagdollState(m_currentState, m_currentPos, m_currentRot);

        // Update the delta position and rotation used for motion extraction.
        if (m_ragdoll->IsSimulated())
        {
            // Accumulate position and rotation delta with each physics update. The actor instance will apply it with the next anim graph update and reset the accumulated delta.
            m_trajectoryDeltaPos += m_currentPos - m_lastPos;
            m_trajectoryDeltaRot = m_trajectoryDeltaRot * m_currentRot * m_lastRot.GetConjugate();
        }
        else
        {
            m_trajectoryDeltaPos = AZ::Vector3::CreateZero();
            m_trajectoryDeltaRot = AZ::Quaternion::CreateIdentity();
        }
    }

    void RagdollInstance::PostAnimGraphUpdate(float timeDelta)
    {
        if (!m_ragdoll)
        {
            return;
        }

        bool disableRagdollQueued = false;

        // Case 1: Ragdoll used this frame and was already used last frame.
        if (m_ragdollUsedThisFrame && m_ragdoll->IsSimulated())
        {
            const Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();
            if (currentPose && currentPose->HasPoseData(azrtti_typeid<PoseDataRagdoll>()))
            {
                const PoseDataRagdoll* ragdollPoseData = currentPose->GetPoseData<PoseDataRagdoll>();
                if (ragdollPoseData)
                {
                    PoseDataRagdoll::FastCopyNodeStates(m_targetState, ragdollPoseData->GetRagdollNodeStates());
                }
            }

            // Set the world space transforms for all kinematic ragdoll nodes after the animation system knows the final pose.
            const size_t ragdollNodeCount = m_jointIndicesByRagdollNodeIndices.size();
            AZ_Assert(ragdollNodeCount == m_ragdoll->GetNumNodes(), "Ragdoll node index to animation skeleton joint index mapping not up to date. Expected the same number of joint indices than ragdoll nodes.");
            for (size_t ragdollNodeIndex = 0; ragdollNodeIndex < ragdollNodeCount; ++ragdollNodeIndex)
            {
                const size_t jointIndex = GetJointIndex(ragdollNodeIndex);
                Physics::RagdollNodeState& ragdollNodeState = m_targetState[ragdollNodeIndex];

                if (ragdollNodeState.m_simulationType == Physics::SimulationType::Kinematic)
                {
                    GetWorldSpaceTransform(currentPose, jointIndex, ragdollNodeState.m_position, ragdollNodeState.m_orientation);
                }
            }

            m_ragdoll->SetStateQueued(m_targetState);
        }
        // Case 2: Do we need to activate the ragdoll?
        else if (m_ragdollUsedThisFrame && !m_ragdoll->IsSimulated())
        {
            // The anim graph got updated, read the current pose from the actor instance.
            ReadRagdollStateFromActorInstance(m_currentState, m_currentPos, m_currentRot);

            if (m_velocityEvaluator)
            {
                m_velocityEvaluator->CalculateInitialVelocities(m_currentState);
            }

            // Activate the ragdoll and set the transforms and initial velocities for the ragdoll nodes.
            m_ragdoll->EnableSimulationQueued(m_currentState);

            m_lastState = m_currentState;
            m_targetState = m_currentState;
            m_lastPos = m_currentPos;
            m_lastRot = m_currentRot;

            m_trajectoryDeltaRot = AZ::Quaternion::CreateIdentity();
            m_trajectoryDeltaPos = AZ::Vector3::CreateZero();
        }
        // Case 3: The ragdoll is not needed anymore, deactivate it.
        else if (!m_ragdollUsedThisFrame && m_ragdoll->IsSimulated())
        {
            m_ragdoll->DisableSimulationQueued();
            disableRagdollQueued = true;
        }

        if (disableRagdollQueued || !m_ragdoll->IsSimulated())
        {
            m_lastState = m_currentState;
            AZ::Vector3 currentPos;
            AZ::Quaternion currentRot;
            ReadRagdollStateFromActorInstance(m_currentState, currentPos, currentRot);

            if (m_velocityEvaluator)
            {
                m_velocityEvaluator->Update(m_lastState, m_currentState, timeDelta);
            }
        }

        // Reset the accumulated motion extraction delta as we applied it this frame in the anim graph update (called before this function).
        ResetTrajectoryDelta();

        // Reset the flag each frame so that we can determine if the ragdoll got used in the next frame.
        m_ragdollUsedLastFrame = m_ragdollUsedThisFrame;
        m_ragdollUsedThisFrame = false;
    }

    const AZ::Outcome<size_t> RagdollInstance::GetRootRagdollNodeIndex() const
    {
        if (m_ragdollRootJoint)
        {
            const AZ::Outcome<size_t> result = GetRagdollNodeIndex(m_ragdollRootJoint->GetNodeIndex());
            AZ_Error("EMotionFX", result.IsSuccess(), "The ragdoll node index for the root node '%s' cannot be found.", m_ragdollRootJoint->GetName());
            return result;
        }
        else
        {
            AZ_Error("EMotionFX", false, "No ragdoll root joint set.");
        }

        return AZ::Failure();
    }

    Physics::Ragdoll* RagdollInstance::GetRagdoll() const
    {
        return m_ragdoll;
    }

    AzPhysics::SceneHandle RagdollInstance::GetRagdollSceneHandle() const
    {
        if (m_ragdoll)
        {
            return m_ragdoll->m_sceneOwner;
        }
        return AzPhysics::InvalidSceneHandle;
    }

    const AZ::Outcome<size_t> RagdollInstance::GetRagdollNodeIndex(size_t jointIndex) const
    {
        const size_t ragdollNodeIndex = m_ragdollNodeIndices[jointIndex];
        if (ragdollNodeIndex == InvalidIndex)
        {
            return AZ::Failure();
        }

        return AZ::Success(ragdollNodeIndex);
    }

    size_t RagdollInstance::GetJointIndex(size_t ragdollNodeIndex) const
    {
        return m_jointIndicesByRagdollNodeIndices[ragdollNodeIndex];
    }

    const AZ::Vector3& RagdollInstance::GetCurrentPos() const
    {
        return m_currentPos;
    }

    const AZ::Vector3& RagdollInstance::GetLastPos() const
    {
        return m_lastPos;
    }

    const AZ::Quaternion& RagdollInstance::GetCurrentRot() const
    {
        return m_currentRot;
    }

    const AZ::Quaternion& RagdollInstance::GetLastRot() const
    {
        return m_lastRot;
    }

    const Physics::RagdollState& RagdollInstance::GetCurrentState() const
    {
        return m_currentState;
    }

    const Physics::RagdollState& RagdollInstance::GetLastState() const
    {
        return m_lastState;
    }

    const Physics::RagdollState& RagdollInstance::GetTargetState() const
    {
        return m_targetState;
    }

    const AZ::Vector3& RagdollInstance::GetTrajectoryDeltaPos() const
    {
        return m_trajectoryDeltaPos;
    }

    const AZ::Quaternion& RagdollInstance::GetTrajectoryDeltaRot() const
    {
        return m_trajectoryDeltaRot;
    }

    void RagdollInstance::ResetTrajectoryDelta()
    {
        m_trajectoryDeltaRot = AZ::Quaternion::CreateIdentity();
        m_trajectoryDeltaPos = AZ::Vector3::CreateZero();
    }

    void RagdollInstance::SetVelocityEvaluator(RagdollVelocityEvaluator* evaluator)
    {
        m_velocityEvaluator = AZStd::unique_ptr<RagdollVelocityEvaluator>(evaluator);
    }

    RagdollVelocityEvaluator* RagdollInstance::GetVelocityEvaluator() const
    {
        return m_velocityEvaluator.get();
    }

    void RagdollInstance::GetWorldSpaceTransform(const Pose* pose, size_t jointIndex, AZ::Vector3& outPosition, AZ::Quaternion& outRotation)
    {
        const Transform& globalTransform = pose->GetModelSpaceTransform(jointIndex);
        const AZ::Quaternion actorInstanceRotation = m_actorInstance->GetLocalSpaceTransform().m_rotation;
        const AZ::Vector3& actorInstanceTranslation = m_actorInstance->GetLocalSpaceTransform().m_position;

        // Calculate the world space position and rotation (The actor instance position and rotation equal the entity transform).
        outPosition = actorInstanceRotation.TransformVector(globalTransform.m_position) + actorInstanceTranslation;
        outRotation = actorInstanceRotation * globalTransform.m_rotation;
    }

    void RagdollInstance::ReadRagdollStateFromActorInstance(Physics::RagdollState& outRagdollState, AZ::Vector3& outRagdollPos, AZ::Quaternion& outRagdollRot)
    {
        AZ_Assert(m_actorInstance, "Expected a valid actor instance.");

        const Actor* actor = m_actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const Pose* currentPose = m_actorInstance->GetTransformData()->GetCurrentPose();

        const AZ::Quaternion& actorInstanceRotation = m_actorInstance->GetLocalSpaceTransform().m_rotation;
        const AZ::Vector3& actorInstanceTranslation = m_actorInstance->GetLocalSpaceTransform().m_position;

        const size_t ragdollNodeCount = m_ragdoll->GetNumNodes();
        outRagdollState.resize(ragdollNodeCount);

        for (size_t ragdollNodeIndex = 0; ragdollNodeIndex < ragdollNodeCount; ++ragdollNodeIndex)
        {
            const size_t jointIndex = m_jointIndicesByRagdollNodeIndices[ragdollNodeIndex];
            const Node* joint = skeleton->GetNode(jointIndex);

            Physics::RagdollNodeState& ragdollNodeState = outRagdollState[ragdollNodeIndex];

            GetWorldSpaceTransform(currentPose, joint->GetNodeIndex(), ragdollNodeState.m_position, ragdollNodeState.m_orientation);
            ragdollNodeState.m_linearVelocity = AZ::Vector3::CreateZero();
            ragdollNodeState.m_angularVelocity = AZ::Vector3::CreateZero();
        }

        if (m_ragdollRootJoint)
        {
            // Calculate the ragdoll world space position and rotation from the ragdoll root node representative in the animation skeleton (e.g. the Pelvis).
            const Transform& globalTransform = currentPose->GetModelSpaceTransform(m_ragdollRootJoint->GetNodeIndex());
            outRagdollPos = actorInstanceRotation.TransformVector(globalTransform.m_position) + actorInstanceTranslation;
            outRagdollRot = actorInstanceRotation * globalTransform.m_rotation;
        }
        else
        {
            AZ_Assert(false, "Expected valid ragdoll root node. Either the ragdoll root node does not exist in the animation skeleton or the ragdoll is empty.");
            outRagdollPos = m_actorInstance->GetLocalSpaceTransform().m_position;
            outRagdollRot = m_actorInstance->GetLocalSpaceTransform().m_rotation;
        }
    }

    void RagdollInstance::ReadRagdollState(Physics::RagdollState& outRagdollState, AZ::Vector3& outRagdollPos, AZ::Quaternion& outRagdollRot)
    {
        AZ_Assert(m_ragdoll && m_actorInstance, "Expected a valid ragdoll and actor instance.");

        if (m_ragdoll->IsSimulated())
        {
            m_ragdoll->GetState(outRagdollState);
            outRagdollPos = m_ragdoll->GetPosition();
            outRagdollRot = m_ragdoll->GetOrientation();
        }
        else
        {
            ReadRagdollStateFromActorInstance(outRagdollState, outRagdollPos, outRagdollRot);
        }
    }

    void RagdollInstance::FindNextRagdollParentForJoint(Node* joint, Node*& outParentJoint, AZ::Outcome<size_t>& outRagdollParentNodeIndex) const
    {
        // Go up the chain and find the next joint that is part of the ragdoll (Parent of the ragdoll node).
        Node* parentCandidateJoint = joint->GetParentNode();
        while (parentCandidateJoint)
        {
            outRagdollParentNodeIndex = GetRagdollNodeIndex(parentCandidateJoint->GetNodeIndex());
            if (outRagdollParentNodeIndex.IsSuccess())
            {
                outParentJoint = parentCandidateJoint;
                return;
            }

            // Iterate, get the parent of the parent.
            parentCandidateJoint = parentCandidateJoint->GetParentNode();
        }

        // Failed, no parent found in the ragdoll.
        outParentJoint = nullptr;
        outRagdollParentNodeIndex = AZ::Failure();
    }

    void RagdollInstance::DebugDraw(DrawLineFunction drawLine) const
    {
        if (!m_actorInstance || !m_ragdoll)
        {
            return;
        }

        if (!m_ragdoll->IsSimulated())
        {
            return;
        }

        const EMotionFX::TransformData* transformData = m_actorInstance->GetTransformData();
        const EMotionFX::Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const size_t jointCount = skeleton->GetNumNodes();

        const RagdollInstance* ragdollInstance = m_actorInstance->GetRagdollInstance();
        const Physics::Ragdoll* ragdoll = ragdollInstance->GetRagdoll();
        const Physics::RagdollState& ragdollCurrentPose = ragdollInstance->GetCurrentState();
        const Physics::RagdollState& ragdollTargetPose = ragdollInstance->GetTargetState();

        static const AZ::Color defaultSimulatedColor = AZ::Color::CreateFromRgba(126, 86, 198, 255);
        static const AZ::Color simulatedTargetColor = AZ::Color::CreateFromRgba(193, 50, 86, 150);
        static const float defaultLineThickness = 10.0f;
        static const float targetLineThickness = 10.0f;
        static const AZ::Color kinematicColor = AZ::Color::CreateFromRgba(255, 216, 0, 255);

        // Why static? This prevents wasting runtime memory to store this pose just for debug rendering as well as runtime allocations.
        // Note: The ragdoll debug rendering won't work in a multi-threaded environment.
        static Pose targetPose;
        targetPose.LinkToActorInstance(m_actorInstance);
        targetPose.InitFromPose(transformData->GetCurrentPose());

        // Read the target pose transforms for all dynamic joints and overwrite the transforms for the output of the anim graph
        // in order to get access to the world space transforms.
        const size_t ragdollNodeCount = ragdoll->GetNumNodes();
        for (size_t i = 0; i < ragdollNodeCount; ++i)
        {
            const size_t jointIndex = ragdollInstance->GetJointIndex(i);
            const Physics::RagdollNodeState& targetJointPose = ragdollTargetPose[i];

            if (targetJointPose.m_simulationType == Physics::SimulationType::Simulated)
            {
                targetPose.SetLocalSpaceTransform(jointIndex, EMotionFX::Transform(targetJointPose.m_position, targetJointPose.m_orientation));
            }
        }

        for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
        {
            const Node* joint = skeleton->GetNode(jointIndex);
            const AZ::Outcome<size_t> ragdollJointIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);
            if (ragdollJointIndex.IsSuccess())
            {
                // Find the parent of the ragdoll node.
                // (There could be several animation skeleton joints on the way that are not part of the ragdoll)
                AZ::Outcome<size_t> ragdollParentJointIndex = AZ::Failure();
                Node* ragdollParentJoint = nullptr;
                Node* parentCandidateJoint = joint->GetParentNode();
                while (parentCandidateJoint)
                {
                    ragdollParentJointIndex = ragdollInstance->GetRagdollNodeIndex(parentCandidateJoint->GetNodeIndex());
                    if (ragdollParentJointIndex.IsSuccess())
                    {
                        ragdollParentJoint = parentCandidateJoint;
                        break;
                    }

                    parentCandidateJoint = parentCandidateJoint->GetParentNode();
                }

                if (ragdollParentJointIndex.IsSuccess())
                {
                    const Physics::RagdollNodeState& currentNodeState = ragdollCurrentPose[ragdollJointIndex.GetValue()];
                    const Physics::RagdollNodeState& currentParentJointPose = ragdollCurrentPose[ragdollParentJointIndex.GetValue()];
                    const AZ::Vector3 currentPos = currentNodeState.m_position;
                    const AZ::Vector3 currentParentPos = currentParentJointPose.m_position;

                    const Physics::RagdollNodeState& targetParentJointPose = ragdollTargetPose[ragdollParentJointIndex.GetValue()];

                    if (targetParentJointPose.m_simulationType == Physics::SimulationType::Simulated)
                    {
                        AZ::Color simulatedColor = defaultSimulatedColor;
                        // TODO: We might want to bake the strength into the alpha channel once we know its range.
                        //simulatedColor.a = static_cast<uint8>(strength * 255.0f);

                        // Render current pose
                        drawLine(currentParentPos, simulatedColor, currentPos, simulatedColor, defaultLineThickness);

                        // Render target pose
                        const AZ::Vector3& targetPos = targetPose.GetWorldSpaceTransform(jointIndex).m_position;
                        const AZ::Vector3& targetParentPos = targetPose.GetWorldSpaceTransform(ragdollParentJoint->GetNodeIndex()).m_position;
                        drawLine(targetParentPos, simulatedTargetColor, targetPos, simulatedTargetColor, targetLineThickness);
                    }
                    else
                    {
                        drawLine(currentParentPos, kinematicColor, currentPos, kinematicColor, defaultLineThickness);
                    }
                }
            }
        }
    }
} // namespace EMotionFX
