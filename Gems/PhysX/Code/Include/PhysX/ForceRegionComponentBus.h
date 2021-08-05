/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace PhysX
{
    /// Requests serviced by a world space force.
    class ForceWorldSpaceRequests
        : public AZ::ComponentBus
    {
    public:
        /// Sets the direction of the force in world space.
        virtual void SetDirection(const AZ::Vector3& direction) = 0;

        /// Gets the direction of the force in world space.
        virtual AZ::Vector3 GetDirection() const = 0;

        /// Sets the magnitude of the force.
        virtual void SetMagnitude(float magnitude) = 0;

        /// Gets the magnitude of the force.
        virtual float GetMagnitude() const = 0;
    };

    using ForceWorldSpaceRequestBus = AZ::EBus<ForceWorldSpaceRequests>;

    /// Requests serviced by a local space force.
    class ForceLocalSpaceRequests
        : public AZ::ComponentBus
    {
    public:
        /// Sets the direction of the force in local space.
        virtual void SetDirection(const AZ::Vector3& direction) = 0;

        /// Gets the direction of the force in local space.
        virtual AZ::Vector3 GetDirection() const = 0;

        /// Sets the magnitude of the force.
        virtual void SetMagnitude(float magnitude) = 0;

        /// Gets the magnitude of the force.
        virtual float GetMagnitude() const = 0;
    };

    using ForceLocalSpaceRequestBus = AZ::EBus<ForceLocalSpaceRequests>;

    /// Requests serviced by a point space force.
    class ForcePointRequests
        : public AZ::ComponentBus
    {
    public:
        /// Sets the magnitude of the force.
        virtual void SetMagnitude(float magnitude) = 0;

        /// Gets the magnitude of the force.
        virtual float GetMagnitude() const = 0;
    };

    using ForcePointRequestBus = AZ::EBus<ForcePointRequests>;

    /// Requests serviced by a spline follow force.
    class ForceSplineFollowRequests
        : public AZ::ComponentBus
    {
    public:
        /// Sets the damping ratio of the force.
        virtual void SetDampingRatio(float ratio) = 0;

        /// Gets the damping ratio of the force.
        virtual float GetDampingRatio() const = 0;

        /// Sets the frequency of the force.
        virtual void SetFrequency(float frequency) = 0;

        /// Gets the frequency of the force.
        virtual float GetFrequency() const = 0;

        /// Sets the traget speed of the force.
        virtual void SetTargetSpeed(float targetSpeed) = 0;

        /// Gets the target speed of the force.
        virtual float GetTargetSpeed() const = 0;

        /// Sets the lookahead of the force.
        virtual void SetLookAhead(float lookAhead) = 0;

        /// Gets the lookahead of the force.
        virtual float GetLookAhead() const = 0;
    };

    using ForceSplineFollowRequestBus = AZ::EBus<ForceSplineFollowRequests>;

    /// Requests serviced by a simple drag force.
    class ForceSimpleDragRequests
        : public AZ::ComponentBus
    {
    public:
        /// Sets the density of the volume.
        virtual void SetDensity(float density) = 0;

        /// Gets the density of the volume.
        virtual float GetDensity() const = 0;
    };

    using ForceSimpleDragRequestBus = AZ::EBus<ForceSimpleDragRequests>;

    /// Requests serviced by a linear damping force.
    class ForceLinearDampingRequests
        : public AZ::ComponentBus
    {
    public:
        /// Sets the damping amount of the force.
        virtual void SetDamping(float damping) = 0;

        /// Gets the damping amount of the force.
        virtual float GetDamping() const = 0;
    };

    using ForceLinearDampingRequestBus = AZ::EBus<ForceLinearDampingRequests>;

    /// Notifications from force regions.
    /// This does not need to be a multi address bus currently as no components are listening to force region events.
    /// Only a global behavior handler is listening and forwarding force region events to script canvas.
    class ForceRegionNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        /// Dispatched when force region computes net force to send impulse on entity.
        virtual void OnCalculateNetForce([[maybe_unused]] AZ::EntityId forceRegionEntityId,
            [[maybe_unused]] AZ::EntityId targetEntityId,
            [[maybe_unused]] const AZ::Vector3& netForceDirection,
            [[maybe_unused]] float netForceMagnitude) {}

        /// Dispatched when any force in force region is changed.
        virtual void OnForceRegionForceChanged([[maybe_unused]] AZ::EntityId forceRegionEntityId) {}
    };
    using ForceRegionNotificationBus = AZ::EBus<ForceRegionNotifications>;

    /// Requests serviced by a force region.
    class ForceRegionRequests
        : public AZ::ComponentBus
    {
    public:
        /// Adds a world space force to the force region.
        /// A world space force region does not account for changes in the entity's transform.
        virtual void AddForceWorldSpace(const AZ::Vector3& direction, float magnitude) = 0;

        /// Adds a local space force to the force region.
        /// A local space force region takes into account changes in the entity's transform.
        virtual void AddForceLocalSpace(const AZ::Vector3& direction, float magnitude) = 0;

        /// Adds a point force to the force region.
        virtual void AddForcePoint(float magnitude) = 0;

        /// Adds a spline follow force to the force region.
        virtual void AddForceSplineFollow(float dampingRatio, float frequency, float targetSpeed, float lookAhead) = 0;

        /// Adds a simple drag force to the force region.
        virtual void AddForceSimpleDrag(float dragCoefficient, float volumeDensity) = 0;

        /// Adds a linear damping force to the force region.
        virtual void AddForceLinearDamping(float damping) = 0;
    };
    using ForceRegionRequestBus = AZ::EBus<ForceRegionRequests>;
}
