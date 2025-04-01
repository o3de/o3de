/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <Editor/SimulatedObjectHelpers.h>
#include <Editor/SimulatedObjectModel.h>
#include <Editor/SkeletonModel.h>
#include <MCore/Source/ReflectionSerializer.h>

namespace EMotionFX
{
    bool SimulatedObjectHelpers::AddSimulatedObject(AZ::u32 actorID, AZStd::optional<AZStd::string> name, MCore::CommandGroup* commandGroup)
    {
        const AZStd::string groupName = AZStd::string::format("Add simulated object");
        return CommandSimulatedObjectHelpers::AddSimulatedObject(actorID, name, commandGroup);
    }

    void SimulatedObjectHelpers::RemoveSimulatedObject(const QModelIndex& modelIndex)
    {
        const size_t objectIndex = modelIndex.data(SimulatedObjectModel::ROLE_OBJECT_INDEX).value<size_t>();
        const Actor* actor = modelIndex.data(SimulatedObjectModel::ROLE_ACTOR_PTR).value<Actor*>();

        const AZStd::string groupName = AZStd::string::format("Remove simulated object");
        MCore::CommandGroup commandGroup(groupName);
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actor->GetID(), objectIndex, &commandGroup);

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    bool SimulatedObjectHelpers::AddSimulatedJoints(const QModelIndexList& modelIndices, size_t objectIndex, bool addChildren, MCore::CommandGroup* commandGroup)
    {
        if (modelIndices.empty())
        {
            return true;
        }

        const Actor* actor = modelIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        AZStd::vector<size_t> jointIndices;

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            jointIndices.emplace_back(joint->GetNodeIndex());
        }

        return CommandSimulatedObjectHelpers::AddSimulatedJoints(actor->GetID(), jointIndices, objectIndex, addChildren, commandGroup);
    }

    void SimulatedObjectHelpers::RemoveSimulatedJoint(const QModelIndex& modelIndex, bool removeChildren)
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        RemoveSimulatedJoints({modelIndex}, removeChildren);
    }

    void SimulatedObjectHelpers::RemoveSimulatedJoints(const QModelIndexList& modelIndices, bool removeChildren)
    {        
        AZStd::unordered_map<size_t, AZStd::pair<const Actor*, AZStd::vector<size_t>>> objectToSkeletonJointIndices;

        for (const QModelIndex& index : modelIndices)
        {
            const bool isJoint = index.data(SimulatedObjectModel::ROLE_JOINT_BOOL).toBool();
            if (!isJoint)
            {
                // Only take the index belongs to a simulated joint.
                continue;
            }
            const Actor* actor = index.data(SimulatedObjectModel::ROLE_ACTOR_PTR).value<Actor*>();
            const size_t objectIndex = static_cast<size_t>(index.data(SimulatedObjectModel::ROLE_OBJECT_INDEX).toInt());
            const size_t jointIndex = index.data(SimulatedObjectModel::ROLE_JOINT_PTR).value<SimulatedJoint*>()->GetSkeletonJointIndex();
            objectToSkeletonJointIndices[objectIndex].first = actor;
            objectToSkeletonJointIndices[objectIndex].second.emplace_back(jointIndex);
        }

        const AZStd::string groupName = "Remove simulated joints";
        MCore::CommandGroup commandGroup(groupName);

        for (const auto& objectIndexAndJointIndices : objectToSkeletonJointIndices)
        {
            const size_t objectIndex = objectIndexAndJointIndices.first;
            const Actor* actor = objectIndexAndJointIndices.second.first;
            const AZStd::vector<size_t> jointIndices = objectIndexAndJointIndices.second.second;

            CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actor->GetID(), jointIndices, objectIndex, removeChildren, &commandGroup);
        }

        AZStd::string result;
        AZ_Error("EMotionFX", CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result), result.c_str());
    }
} // namespace EMotionFX
