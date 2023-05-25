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
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulatorCommandCallback.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    enum class JointLimitFrame : AZ::u8
    {
        Parent,
        Child
    };

    //! Provides functionality for interactively editing character physics joint limit frame orientations in the Animation Editor Viewport. 
    class JointLimitRotationManipulators
        : public PhysicsSetupManipulatorsBase
        , private AZ::TickBus::Handler
        , private PhysicsSetupManipulatorRequestBus::Handler
    {
    public:
        JointLimitRotationManipulators(JointLimitFrame jointLimitFrame);
        void Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData) override;
        void Refresh() override;
        void Teardown() override;
        void ResetValues() override;
        void InvalidateEditorValues() override;

    private:
        // AZ::TickBus::Handler overrides ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;

        // PhysicsSetupManipulatorRequestBus::Handler overrides ...
        void OnUnderlyingPropertiesChanged() override;

        void OnManipulatorMoved(const AZ::Quaternion& rotation);
        void BeginEditing();
        void EndEditing();

        AZ::Quaternion& GetLocalOrientation();
        const AZ::Quaternion& GetLocalOrientation() const;

        AzToolsFramework::RotationManipulators m_rotationManipulators =
            AzToolsFramework::RotationManipulators(AZ::Transform::CreateIdentity());
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        JointLimitFrame m_jointLimitFrame = JointLimitFrame::Parent;
        MCore::CommandGroup m_commandGroup;
        AZStd::unique_ptr<PhysicsSetupManipulatorCommandCallback> m_adjustJointLimitCallback;
    };

    void CreateCommandAdjustJointLimit(MCore::CommandGroup& commandGroup, const PhysicsSetupManipulatorData& physicsSetupManipulatorData);
    void ExecuteCommandAdjustJointLimit(MCore::CommandGroup& commandGroup, const PhysicsSetupManipulatorData& physicsSetupManipulatorData);
} // namespace EMotionFX
