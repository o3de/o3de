/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulatorCommandCallback.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    //! Used for storing the initial state of the joint twist limits.
    struct JointTwistLimitState
    {
        AZStd::optional<float> m_twistLimitLower;
        AZStd::optional<float> m_twistLimitUpper;
    };

    //! Provides functionality for interactively editing character physics joint limit extents in the Animation Editor Viewport. 
    class JointTwistLimitManipulators
        : public PhysicsSetupManipulatorsBase
        , private AZ::TickBus::Handler
        , private PhysicsSetupManipulatorRequestBus::Handler
    {
    public:
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

        void BeginEditing();
        void EndEditing();

        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_twistLimitLowerManipulator;
        AZStd::shared_ptr<AzToolsFramework::AngularManipulator> m_twistLimitUpperManipulator;
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        MCore::CommandGroup m_commandGroup;
        AZStd::unique_ptr<PhysicsSetupManipulatorCommandCallback> m_adjustJointLimitCallback;
        JointTwistLimitState m_jointTwistLimitState;
        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;
    };
} // namespace EMotionFX
