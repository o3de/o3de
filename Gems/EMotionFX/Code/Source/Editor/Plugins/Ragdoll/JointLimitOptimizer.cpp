/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <EMotionFX/CommandSystem/Source/JointLimitCommands.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/Plugins/Ragdoll/JointLimitOptimizer.h>
#include <Editor/Plugins/Ragdoll/JointLimitRotationManipulators.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    void OptimizeJointLimits(const PhysicsSetupManipulatorData& physicsSetupManipulatorData)
    {
        if (!physicsSetupManipulatorData.m_actor || !physicsSetupManipulatorData.m_node ||
            !physicsSetupManipulatorData.m_jointConfiguration)
        {
            EMStudio::GetNotificationWindowManager()->CreateNotificationWindow(
                EMStudio::NotificationWindow::TYPE_ERROR, "Joint limit optimization <font color=red>failed</font>");
            return;
        }

        auto* jointHelpersInterface = AZ::Interface<AzPhysics::JointHelpersInterface>::Get();
        auto* editorJointHelpersInterface = AZ::Interface<AzPhysics::EditorJointHelpersInterface>::Get();
        if (!jointHelpersInterface || !editorJointHelpersInterface)
        {
            EMStudio::GetNotificationWindowManager()->CreateNotificationWindow(
                EMStudio::NotificationWindow::TYPE_ERROR,
                "Joint limit optimization <font color=red>failed</font> due to missing physics joint interface");
            return;
        }

        // get initial guess to start the solver from
        const AZ::Vector3 boneDirection =
            GetBoneDirection(physicsSetupManipulatorData.m_actor->GetSkeleton(), physicsSetupManipulatorData.m_node);
        const Pose* bindPose = physicsSetupManipulatorData.m_actor->GetSkeleton()->GetBindPose();
        const Transform& nodeBindTransform = bindPose->GetModelSpaceTransform(physicsSetupManipulatorData.m_node->GetNodeIndex());
        const Transform& parentBindTransform = physicsSetupManipulatorData.m_node->GetParentNode()
            ? bindPose->GetModelSpaceTransform(physicsSetupManipulatorData.m_node->GetParentIndex())
            : Transform::CreateIdentity();
        const AZ::Quaternion& nodeBindRotationWorld = nodeBindTransform.m_rotation;
        const AZ::Quaternion& parentBindRotationWorld = parentBindTransform.m_rotation;
        AZStd::unique_ptr<AzPhysics::JointConfiguration> jointInitialConfiguration =
            jointHelpersInterface->ComputeInitialJointLimitConfiguration(
                physicsSetupManipulatorData.m_jointConfiguration->RTTI_GetType(),
                parentBindRotationWorld,
                nodeBindRotationWorld,
                boneDirection,
                {});

        if (!jointInitialConfiguration)
        {
            EMStudio::GetNotificationWindowManager()->CreateNotificationWindow(
                EMStudio::NotificationWindow::TYPE_ERROR,
                "Computing initial configuration for joint limit optimization <font color=red>failed</font>");
            return;
        }

        // check how many motions are available to help decide how many samples to take per motion
        MotionManager& motionManager = EMotionFX::GetMotionManager();
        const size_t numMotionSets = motionManager.GetNumMotionSets();
        int numMotions = 0;
        for (size_t motionSetIndex = 0; motionSetIndex < numMotionSets; motionSetIndex++)
        {
            const MotionSet* motionSet = motionManager.GetMotionSet(motionSetIndex);
            const MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
            for (const auto& motionEntryPair : motionEntries)
            {
                MotionSet::MotionEntry* motionEntry = motionEntryPair.second;
                motionSet->LoadMotion(motionEntry);
                const Motion* motion = motionEntry->GetMotion();
                if (!motion || !motion->GetMotionData())
                {
                    continue;
                }
                const AZ::Outcome<size_t> jointIndexOutcome =
                    motion->GetMotionData()->FindJointIndexByName(physicsSetupManipulatorData.m_node->GetNameString());
                numMotions += jointIndexOutcome.IsSuccess();
            }
        }

        if (numMotions == 0)
        {
            EMStudio::GetNotificationWindowManager()->CreateNotificationWindow(
                EMStudio::NotificationWindow::TYPE_WARNING,
                "Please ensure a motion set is loaded in order to perform joint limit optimization");
            return;
        }

        // now do the actual sampling of rotation values
        const int numSamplesPerMotion = AZ::GetMin(JointLimitOptimizerMaxSamplesPerMotion, JointLimitOptimizerTotalSamples / numMotions);
        const int sampleCount = numMotions * numSamplesPerMotion;
        AZStd::vector<AZ::Quaternion> localRotationSamples;
        localRotationSamples.reserve(sampleCount);
        for (size_t motionSetIndex = 0; motionSetIndex < numMotionSets; motionSetIndex++)
        {
            const MotionSet* motionSet = motionManager.GetMotionSet(motionSetIndex);
            const MotionSet::MotionEntries& motionEntries = motionSet->GetMotionEntries();
            for (const auto& motionEntryPair : motionEntries)
            {
                MotionSet::MotionEntry* motionEntry = motionEntryPair.second;
                motionSet->LoadMotion(motionEntry);
                const Motion* motion = motionEntry->GetMotion();
                if (!motion || !motion->GetMotionData())
                {
                    continue;
                }
                const MotionData* motionData = motion->GetMotionData();
                const AZ::Outcome<size_t> jointIndexOutcome =
                    motionData->FindJointIndexByName(physicsSetupManipulatorData.m_node->GetNameString());
                if (jointIndexOutcome.IsSuccess())
                {
                    const float duration = motion->GetDuration();
                    for (int sampleIndex = 0; sampleIndex < numSamplesPerMotion; sampleIndex++)
                    {
                        localRotationSamples.push_back(motionData->SampleJointRotation(
                            (sampleIndex * duration) / aznumeric_cast<float>(numSamplesPerMotion), jointIndexOutcome.GetValue()));
                    }
                }
            }
        }

        // perform the optimization
        AZStd::unique_ptr<AzPhysics::JointConfiguration> optimizedJointLimit =
            editorJointHelpersInterface->ComputeOptimalJointLimit(jointInitialConfiguration.get(), localRotationSamples);

        if (!optimizedJointLimit)
        {
            EMStudio::GetNotificationWindowManager()->CreateNotificationWindow(
                EMStudio::NotificationWindow::TYPE_ERROR, "Joint limit optimization <font color=red>failed</font>");
            return;
        }

        MCore::CommandGroup commandGroup;
        commandGroup.SetGroupName("Adjust joint limit");
        const AZ::u32 actorId = physicsSetupManipulatorData.m_actor->GetID();
        const AZStd::string& nodeName = physicsSetupManipulatorData.m_node->GetNameString();
        CommandAdjustJointLimit* command = aznew CommandAdjustJointLimit(actorId, nodeName);
        commandGroup.AddCommand(command);
        command->SetOldJointConfiguration(physicsSetupManipulatorData.m_jointConfiguration);
        command->SetJointConfiguration(optimizedJointLimit.get());
        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result);
        commandGroup.Clear();
    }
} // namespace EMotionFX
