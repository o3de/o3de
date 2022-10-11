/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ComponentModes/CapsuleViewportEdit.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulatorCommandCallback.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>

namespace EMotionFX
{
    //! Provides functionality for interactively editing character physics capsule collider dimensions in the Animation Editor Viewport.
    class ColliderCapsuleManipulators
        : public PhysicsSetupManipulatorsBase
        , private AZ::TickBus::Handler
        , private PhysicsSetupManipulatorRequestBus::Handler
        , private AzToolsFramework::CapsuleViewportEdit
    {
    public:
        // PhysicsSetupManipulatorsBase overrides ...
        void Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData) override;
        void Refresh() override;
        void Teardown() override;
        void ResetValues() override;

    private:
        // AZ::TickBus::Handler overrides ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;

        // PhysicsSetupManipulatorRequestBus::Handler overrides ...
        void OnUnderlyingPropertiesChanged() override;

        // CapsuleViewportEdit overrides ...
        AZ::Transform GetCapsuleWorldTransform() const override;
        AZ::Transform GetCapsuleLocalTransform() const override;
        AZ::Vector3 GetCapsuleNonUniformScale() const override;
        float GetCapsuleRadius() const override;
        float GetCapsuleHeight() const override;
        void SetCapsuleRadius(float radius) override;
        void SetCapsuleHeight(float height) override;
        void BeginEditing() override;
        void FinishEditing() override;

        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        MCore::CommandGroup m_commandGroup;
        AZStd::unique_ptr<PhysicsSetupManipulatorCommandCallback> m_adjustColliderCallback;
    };
} // namespace EMotionFX
