/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulatorCommandCallback.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    //! Provides functionality for interactively editing character physics collider positions in the Animation Editor Viewport. 
    class ColliderTranslationManipulators
        : public PhysicsSetupManipulatorsBase
        , private PhysicsSetupManipulatorRequestBus::Handler
    {
    public:
        ColliderTranslationManipulators();
        void Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData) override;
        void Refresh() override;
        void Teardown() override;
        void ResetValues() override;

    private:
        AZ::Vector3 GetPosition(const AZ::Vector3& startPosition, const AZ::Vector3& offset) const;
        void OnManipulatorMoved(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void BeginEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void EndEditing(const AZ::Vector3& startPosition, const AZ::Vector3& offset);

        // PhysicsSetupManipulatorRequestBus::Handler overrides ...
        void OnUnderlyingPropertiesChanged() override;

        MCore::CommandGroup m_commandGroup;
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        AzToolsFramework::TranslationManipulators m_translationManipulators;
        AZStd::unique_ptr<PhysicsSetupManipulatorCommandCallback> m_adjustColliderCallback;
    };
} // namespace EMotionFX
