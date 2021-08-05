/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Skeleton.h>
#include <Source/Editor/SimulatedObjectModel.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <Source/Editor/QtMetaTypes.h>

namespace EMotionFX
{
    bool SimulatedObjectModel::CommandAddSimulatedObjectPreCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->PreAddObject();
        return true;
    }

    bool SimulatedObjectModel::CommandAddSimulatedObjectPreCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        CommandAddSimulatedObject* commandAddSimulatedObject = static_cast<CommandAddSimulatedObject*>(command);
        m_simulatedObjectModel->PreRemoveObject(commandAddSimulatedObject->GetObjectIndex());
        return true;
    }

    bool SimulatedObjectModel::CommandAddSimulatedObjectPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->PostAddObject();
        return true;
    }

    bool SimulatedObjectModel::CommandAddSimulatedObjectPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);

        m_simulatedObjectModel->PostRemoveObject();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedObjectPreCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        CommandRemoveSimulatedObject* commandRemoveSimulatedObject = static_cast<CommandRemoveSimulatedObject*>(command);
        m_simulatedObjectModel->PreRemoveObject(commandRemoveSimulatedObject->GetObjectIndex());
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedObjectPreCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->PreAddObject();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedObjectPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->PostRemoveObject();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedObjectPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->PostAddObject();
        return true;
    }

    bool SimulatedObjectModel::CommandAdjustSimulatedObjectPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        CommandAdjustSimulatedObject* commandAdjustSimulatedObject = static_cast<CommandAdjustSimulatedObject*>(command);
        const QModelIndex objectIndex = m_simulatedObjectModel->index(static_cast<int>(commandAdjustSimulatedObject->GetObjectIndex()), 0);
        m_simulatedObjectModel->dataChanged(objectIndex, objectIndex.sibling(objectIndex.row(), m_simulatedObjectModel->columnCount() - 1));
        return true;
    }

    bool SimulatedObjectModel::CommandAdjustSimulatedObjectPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return this->Execute(command, commandLine);
    }

    bool SimulatedObjectModel::CommandAddSimulatedJointsPreCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->beginResetModel();
        m_simulatedObjectModel->GetSelectionModel()->clearSelection();

        return true;
    }

    bool SimulatedObjectModel::CommandAddSimulatedJointsPreCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->beginResetModel();
        m_simulatedObjectModel->GetSelectionModel()->clearSelection();

        return true;
    }

    bool SimulatedObjectModel::CommandAddSimulatedJointsPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->endResetModel();
        return true;
    }

    bool SimulatedObjectModel::CommandAddSimulatedJointsPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->endResetModel();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedJointsPreCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->beginResetModel();
        m_simulatedObjectModel->GetSelectionModel()->clearSelection();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedJointsPreCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->beginResetModel();
        m_simulatedObjectModel->GetSelectionModel()->clearSelection();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedJointsPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->endResetModel();
        return true;
    }

    bool SimulatedObjectModel::CommandRemoveSimulatedJointsPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_simulatedObjectModel->endResetModel();
        return true;
    }

    bool SimulatedObjectModel::CommandAdjustSimulatedJointPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);

        CommandAdjustSimulatedJoint* commandAdjustSimulatedJoint = static_cast<CommandAdjustSimulatedJoint*>(command);
        const QModelIndexList foundIndexes = m_simulatedObjectModel->match(
            /*start=*/m_simulatedObjectModel->index(0, 0),
            /*role=*/SimulatedObjectModel::ROLE_JOINT_PTR,
            /*value=*/QVariant::fromValue(commandAdjustSimulatedJoint->GetSimulatedJoint()),
            /*hits=*/1,
            /*flags=*/Qt::MatchExactly | Qt::MatchRecursive
        );
        if (foundIndexes.empty())
        {
            return false;
        }
        const QModelIndex jointIndex = foundIndexes[0];
        m_simulatedObjectModel->dataChanged(jointIndex, jointIndex.sibling(jointIndex.row(), m_simulatedObjectModel->columnCount() - 1));
        return true;
    }

    bool SimulatedObjectModel::CommandAdjustSimulatedJointPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        return this->Execute(command, commandLine);
    }
} // namespace EMotionFX
