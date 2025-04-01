/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace PhysX
{
    //! Bus for requests to the example character gameplay component.
    class CharacterGameplayRequests
        : public AZ::ComponentBus
    {
    public:
        //! Returns whether the character is standing on the ground (or another object which will prevent downward motion).
        virtual bool IsOnGround() const = 0;

        //! Gets the gravity multipier.
        //! The gravity multiplier is combined with the gravity value for the physics world to which the character
        //! belongs when applying gravity to the character.
        virtual float GetGravityMultiplier() const = 0;

        //! Sets the gravity multipier.
        //! The gravity multiplier is combined with the gravity value for the physics world to which the character
        //! belongs when applying gravity to the character.
        //! @param gravityMultiplier The new gravity multiplier value.
        virtual void SetGravityMultiplier(float gravityMultiplier) = 0;

        //! Gets the vertical size of the box used when checking for ground contact.
        virtual float GetGroundDetectionBoxHeight() const = 0;

        //! Sets the vertical size of the box used when checking for ground contact.
        //! @param groundDetectionBoxHeight The new ground detection box height value.
        virtual void SetGroundDetectionBoxHeight(float groundDetectionBoxHeight) = 0;

        //! Gets the falling velocity.
        virtual AZ::Vector3 GetFallingVelocity() const = 0;

        //! Sets the falling velocity.
        //! An example where this might be used is if gravity is disabled during a jump animation (because the character
        //! is then completely animation-driven and the up and down movement comes from the animation), then when the jump
        //! finishes gravity is re-enabled and an initial downwards velocity is needed to match the end of the animation.
        virtual void SetFallingVelocity(const AZ::Vector3& fallingVelocity) = 0;
    };
    using CharacterGameplayRequestBus = AZ::EBus<CharacterGameplayRequests>;
} // namespace PhysX
