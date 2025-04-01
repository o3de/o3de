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
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    //! Provides functionality for interactively editing character physics collider orientations in the Animation Editor Viewport. 
    class ColliderRotationManipulators
        : public PhysicsSetupManipulatorsBase
        , private AZ::TickBus::Handler
        , private PhysicsSetupManipulatorRequestBus::Handler
    {
    public:
        ColliderRotationManipulators();
        void Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData) override;
        void Refresh() override;
        void Teardown() override;
        void ResetValues() override;

    private:
        // AZ::TickBus::Handler overrides ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;

        // PhysicsSetupManipulatorRequestBus::Handler overrides ...
        void OnUnderlyingPropertiesChanged() override;

        void OnManipulatorMoved(const AZ::Quaternion& rotation);
        void BeginEditing(const AZ::Quaternion& rotation);
        void EndEditing(const AZ::Quaternion& rotation);

        AzToolsFramework::RotationManipulators m_rotationManipulators;
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        MCore::CommandGroup m_commandGroup;
        AZStd::unique_ptr<PhysicsSetupManipulatorCommandCallback> m_adjustColliderCallback;
    };
} // namespace EMotionFX
