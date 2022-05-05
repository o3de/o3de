/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Transform.h>

namespace Physics
{
    class CharacterColliderNodeConfiguration;
} // namespace Physics

namespace EMotionFX
{
    class Actor;
    class Node;

    struct RagdollManipulatorData
    {
        AZ::Transform m_nodeWorldTransform = AZ::Transform::CreateIdentity();
        Physics::CharacterColliderNodeConfiguration* m_colliderNodeConfiguration = nullptr;
        bool m_valid = false;
    };

    //! Base class for various manipulator modes, e.g. collider translation, collider orientation, etc.
    class RagdollManipulatorsBase
    {
    public:
        virtual ~RagdollManipulatorsBase() = default;

        //! Called when the manipulator mode is entered to initialize the mode.
        virtual void Setup(RagdollManipulatorData& ragdollManipulatorData) = 0;

        //! Called when the manipulator mode needs to refresh its values.
        virtual void Refresh(RagdollManipulatorData& ragdollManipulatorData) = 0;

        //! Called when the manipulator mode exits to perform cleanup.
        virtual void Teardown() = 0;

        //! Called when reset hot key is pressed.
        //! Should reset values in the manipulator mode to sensible defaults.
        virtual void ResetValues(RagdollManipulatorData& ragdollManipulatorData) = 0;
    };
} // namespace EMotionFX
