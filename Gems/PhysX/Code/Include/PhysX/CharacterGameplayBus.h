/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        //! @param gravityMultiplier The new 
        virtual void SetGravityMultiplier(float gravityMultiplier) = 0;

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
