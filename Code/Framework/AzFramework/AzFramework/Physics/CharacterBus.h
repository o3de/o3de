/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Vector3;
}

namespace Physics
{
    class Character;

    /// Messages serviced by character controllers.
    class CharacterRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~CharacterRequests() = default;

        /// Gets the base position of the character.
        virtual AZ::Vector3 GetBasePosition() const = 0;

        /// Directly moves (teleports) the character to a new base position.
        /// @param position The new base position for the character.
        virtual void SetBasePosition(const AZ::Vector3& position) = 0;

        /// Gets the position of the center of the character.
        virtual AZ::Vector3 GetCenterPosition() const = 0;

        /// Gets the step height (the parameter which affects how high the character can step).
        virtual float GetStepHeight() const = 0;

        /// Sets the step height (the parameter which affects how high the character can step).
        /// @param stepHeight The new value for the step height parameter.
        virtual void SetStepHeight(float stepHeight) = 0;

        /// Gets the character's up direction (the direction used by various controller logic, for example stepping).
        virtual AZ::Vector3 GetUpDirection() const = 0;

        /// Sets the character's up direction (the direction used by various controller logic, for example stepping).
        /// @param upDirection The new value for the up direction.
        virtual void SetUpDirection(const AZ::Vector3& upDirection) = 0;

        /// Gets the maximum slope which the character can climb, in degrees.
        virtual float GetSlopeLimitDegrees() const = 0;

        /// Sets the maximum slope which the character can climb, in degrees.
        /// @param slopeLimitDegrees the new slope limit value (in degrees, should be between 0 and 90).
        virtual void SetSlopeLimitDegrees(float slopeLimitDegrees) = 0;

        /// Gets the maximum speed.
        /// If the accumulated requested velocity for the character exceeds this magnitude, it will be clamped.
        virtual float GetMaximumSpeed() const = 0;

        /// Sets the maximum speed.
        /// If the accumulated requested velocity for the character exceeds this magnitude, it will be clamped.
        /// Values below 0 will be treated as 0.
        /// @param maximumSpeed The new value for the maximum speed.
        virtual void SetMaximumSpeed(float maximumSpeed) = 0;

        /// Gets the observed velocity of the character, which may differ from the desired velocity if the character is obstructed.
        virtual AZ::Vector3 GetVelocity() const = 0;

        /// Queues up a request to apply a velocity to the character.
        /// All requests received during a tick are accumulated (so for example, the effects of animation and gravity
        /// can be applied in two separate requests), and a movement with the accumulated velocity is performed once
        /// per tick, prior to the physics update.
        /// Obstacles may prevent the actual movement from exactly matching the requested movement.
        virtual void AddVelocity(const AZ::Vector3& velocity) = 0;

        /// Check if there is a character physics component present.
        /// Return true in the request handler implementation in order for things like the animation system to work properly.
        virtual bool IsPresent() const { return false; }

        /// Gets a pointer to the Character object owned by the controller.
        virtual Character* GetCharacter() = 0;
    };

    using CharacterRequestBus = AZ::EBus<CharacterRequests>;
} // namespace Physics
