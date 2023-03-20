/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Transform.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulatorBus.h>

namespace Physics
{
    class CharacterColliderNodeConfiguration;
} // namespace Physics

namespace AzPhysics
{
    struct JointConfiguration;
} // namespace AzPhysics

namespace EMotionFX
{
    class Actor;
    class Node;
    class ObjectEditor;
    class ColliderContainerWidget;
    class RagdollJointLimitWidget;

    struct PhysicsSetupManipulatorData
    {
        bool HasColliders() const;
        bool HasCapsuleCollider() const;
        bool HasJointLimit() const;
        AZ::Transform GetJointParentFrameWorld() const;

        AZ::Transform m_nodeWorldTransform = AZ::Transform::CreateIdentity();
        AZ::Transform m_parentWorldTransform = AZ::Transform::CreateIdentity();
        Physics::CharacterColliderNodeConfiguration* m_colliderNodeConfiguration = nullptr;
        AzPhysics::JointConfiguration* m_jointConfiguration = nullptr;
        Actor* m_actor = nullptr;
        Node* m_node = nullptr;
        ColliderContainerWidget* m_collidersWidget = nullptr;
        RagdollJointLimitWidget* m_jointLimitWidget = nullptr;
        bool m_valid = false;
    };

    //! Base class for various manipulator modes, e.g. collider translation, collider orientation, etc.
    class PhysicsSetupManipulatorsBase
    {
    public:
        virtual ~PhysicsSetupManipulatorsBase() = default;

        //! Called when the manipulator mode is entered to initialize the mode.
        virtual void Setup(const PhysicsSetupManipulatorData& physicsSetupManipulatorData) = 0;

        //! Called when the manipulator mode needs to refresh its values.
        virtual void Refresh() = 0;

        //! Called when the manipulator mode exits to perform cleanup.
        virtual void Teardown() = 0;

        //! Called when reset hot key is pressed.
        //! Should reset values in the manipulator mode to sensible defaults.
        virtual void ResetValues() = 0;

        //! Causes values in associated property editor to refresh.
        virtual void InvalidateEditorValues()
        {
        }

        void SetViewportId(AZ::s32 viewportId);
    protected:
        AZ::s32 m_viewportId;
    };

    //! Used when null mode is selected.
    class PhysicsSetupManipulatorsNull
        : public PhysicsSetupManipulatorsBase
    {
    public:
        void Setup([[maybe_unused]] const PhysicsSetupManipulatorData& physicsSetupManipulatorData) override {};
        void Refresh() override {};
        void Teardown() override {};
        void ResetValues() override {};
    };
} // namespace EMotionFX
