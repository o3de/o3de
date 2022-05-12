/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    class ColliderRotationManipulators
        : public PhysicsSetupManipulatorsBase
        , private AZ::TickBus::Handler
        , private PhysicsSetupManipulatorRequestBus::Handler
    {
    public:
        ColliderRotationManipulators();
        ~ColliderRotationManipulators();
        void Setup(PhysicsSetupManipulatorData& physicsSetupManipulatorData) override;
        void Refresh() override;
        void Teardown() override;
        void ResetValues() override;

    private:
        // AZ::TickBus::Handler ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;

        AZ::s32 GetViewportId();

        void OnManipulatorMoved(const AZ::Quaternion& rotation);
        void BeginEditing(const AZ::Quaternion& rotation);
        void FinishEditing(const AZ::Quaternion& rotation);

        // PhysicsSetupManipulatorRequestBus::Handler ...
        void OnUnderlyingPropertiesChanged() override;

        class DEFINECOMMANDCALLBACK_API DataChangedCallback : public MCore::Command::Callback
        {
            MCORE_MEMORYOBJECTCATEGORY(DataChangedCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM);

        public:
            explicit DataChangedCallback(ColliderRotationManipulators* manipulators, bool executePreUndo, bool executePreCommand = false)
                : MCore::Command::Callback(executePreUndo, executePreCommand)
                , m_manipulators(manipulators)
            {
            }
            bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine) override;
            bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine) override;

        private:
            ColliderRotationManipulators* m_manipulators{};
        };

        AzToolsFramework::RotationManipulators m_rotationManipulators;
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        AZStd::optional<AZ::s32> m_viewportId;
        MCore::CommandGroup m_commandGroup;
        AZStd::unique_ptr<DataChangedCallback> m_adjustColliderCallback;

        friend class DataChangedCallback;
    };
} // namespace EMotionFX
