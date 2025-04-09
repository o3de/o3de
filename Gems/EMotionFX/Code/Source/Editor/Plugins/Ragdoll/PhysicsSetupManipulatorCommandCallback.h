/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <MCore/Source/Command.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>

namespace EMotionFX
{
    class DEFINECOMMANDCALLBACK_API PhysicsSetupManipulatorCommandCallback : public MCore::Command::Callback
    {
        MCORE_MEMORYOBJECTCATEGORY(
            PhysicsSetupManipulatorCommandCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM);

    public:
        explicit PhysicsSetupManipulatorCommandCallback(
            PhysicsSetupManipulatorsBase* manipulators, bool executePreUndo, bool executePreCommand = false)
            : MCore::Command::Callback(executePreUndo, executePreCommand)
            , m_manipulators(manipulators)
        {
        }
        bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine) override;
        bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine) override;

    private:
        PhysicsSetupManipulatorsBase* m_manipulators{};
    };
} // namespace EMotionFX
