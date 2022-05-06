/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <Editor/Plugins/Ragdoll/RagdollManipulators.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    class RagdollColliderTranslationManipulators : public RagdollManipulatorsBase
    {
    public:
        RagdollColliderTranslationManipulators();
        ~RagdollColliderTranslationManipulators();
        void Setup(RagdollManipulatorData& ragdollManipulatorData) override;
        void Refresh() override;
        void Teardown() override;
        void ResetValues() override;

    private:
        AZ::Vector3 GetPosition(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void BeginEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void FinishEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset);

        MCore::CommandGroup m_commandGroup;
        RagdollManipulatorData m_ragdollManipulatorData;
        AzToolsFramework::TranslationManipulators m_translationManipulators;

        class DEFINECOMMANDCALLBACK_API DataChangedCallback : public MCore::Command::Callback
        {
            MCORE_MEMORYOBJECTCATEGORY(DataChangedCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MCore::MCORE_MEMCATEGORY_COMMANDSYSTEM);

        public:
            explicit DataChangedCallback(
                RagdollColliderTranslationManipulators* manipulators, bool executePreUndo, bool executePreCommand = false)
                : MCore::Command::Callback(executePreUndo, executePreCommand)
                , m_manipulators(manipulators)
            {
            }
            bool Execute(MCore::Command* command, const MCore::CommandLine& commandLine) override;
            bool Undo(MCore::Command* command, const MCore::CommandLine& commandLine) override;

        private:
            RagdollColliderTranslationManipulators* m_manipulators{};
        };

        DataChangedCallback* m_adjustColliderCallback;

        friend class DataChangedCallback;
    };
} // namespace EMotionFX
