/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Vector3.h>

namespace NvCloth
{
    //! Interface to configure cloth parameters that define its behavior during simulation.
    //!
    //! @note Use ICloth interface to obtain a IClothConfigurator.
    class IClothConfigurator
    {
    public:
        //! Sets world transform to cloth.
        virtual void SetTransform(const AZ::Transform& transformWorld) = 0;

        //! Clears inertia derived from SetTransform to zero.
        virtual void ClearInertia() = 0;

        //! Mass scale applied to all particles, zero makes all particles static.
        //!
        //! @note This function results in a copy of all particle data
        //!       to NvCloth and therefore it's not a fast operation.
        virtual void SetMass(float mass) = 0;

        //! Gravity applied to cloth during simulation.
        virtual void SetGravity(const AZ::Vector3& gravity) = 0;

        //! Stiffness exponent per second applied to damping, damping dragging,
        //! wind dragging, wind lifting, self collision stiffness, fabric stiffness,
        //! fabric compression, fabric stretch, tether constraint stiffness and
        //! motion constraints stiffness.
        //! Stiffness frequency valid values are > 0.0.
        virtual void SetStiffnessFrequency(float frequency) = 0;

        //! Damping of particles' velocity.
        //! 0.0: Velocity is unaffected.
        //! 1.0: Velocity is zeroed.
        virtual void SetDamping(const AZ::Vector3& damping) = 0;

        //! Portion of velocity applied to particles.
        //! 0.0: Particles is unaffected.
        //! 1.0: Damped global particle velocity.
        virtual void SetDampingLinearDrag(const AZ::Vector3& linearDrag) = 0;

        //! Portion of angular velocity applied to turning particles.
        //! 0.0: Particles is unaffected.
        //! 1.0: Damped global particle angular velocity.
        virtual void SetDampingAngularDrag(const AZ::Vector3& angularDrag) = 0;

        //! Portion of acceleration applied to particles.
        //! 0.0: Particles are unaffected.
        //! 1.0: Physically correct.
        virtual void SetLinearInertia(const AZ::Vector3& linearInertia) = 0;

        //! Portion of angular acceleration applied to turning particles.
        //! 0.0: Particles are unaffected.
        //! 1.0: Physically correct.
        virtual void SetAngularInertia(const AZ::Vector3& angularInertia) = 0;

        //! Portion of angular velocity applied to turning particles.
        //! 0.0: Particles are unaffected.
        //! 1.0: Physically correct.
        virtual void SetCentrifugalInertia(const AZ::Vector3& centrifugalInertia) = 0;

        //! Wind in global coordinates acting on cloth's triangles.
        //! Disabled when both wind air coefficients are zero.
        //! Wind velocity range is [-50.0, 50.0].
        //! @note A combination of high values in wind properties can cause unstable results.
        virtual void SetWindVelocity(const AZ::Vector3& velocity) = 0;

        //! Amount of air drag.
        //! Wind drag range is [0.0, 1.0].
        //! @note A combination of high values in wind properties can cause unstable results.
        virtual void SetWindDragCoefficient(float drag) = 0;

        //! Amount of air lift.
        //! Wind lift range is [0.0, 1.0].
        //! @note A combination of high values in wind properties can cause unstable results.
        virtual void SetWindLiftCoefficient(float lift) = 0;

        //! Density of air used for air drag and lift calculations.
        //! Fluid density valid values are > 0.0.
        //! @note A combination of high values in wind properties can cause unstable results.
        virtual void SetWindFluidDensity(float density) = 0;

        //! Amount of friction with colliders.
        //! 0.0: No friction.
        //! Friction valid values are >= 0.0.
        virtual void SetCollisionFriction(float friction) = 0;

        //! Controls how quickly mass is increased during collisions.
        //! 0.0: No mass scaling.
        //! Scale valid values are >= 0.0.
        virtual void SetCollisionMassScale(float scale) = 0;

        //! Enables/Disables continuous collision detection.
        //! Enabling it improves collision by computing time of impact between cloth particles and colliders.
        //! @note The increase in quality comes with a cost in performance,
        //!       it's recommended to use only when required.
        virtual void EnableContinuousCollision(bool value) = 0;

        //! Enables/Disables colliders affecting static particles (inverse mass 0.0).
        virtual void SetCollisionAffectsStaticParticles(bool value) = 0;

        //! Meters that particles need to be separated from each other.
        //! 0.0: No self collision.
        //! Distance valid values are > 0.0.
        virtual void SetSelfCollisionDistance(float distance) = 0;

        //! Stiffness for the self collision constraints.
        //! 0.0: No self collision.
        //! Stiffness range is [0.0, 1.0].
        virtual void SetSelfCollisionStiffness(float stiffness) = 0;

        //! Configures vertical constraints parameters.
        //!
        //! @note Applicable only if cloth has vertical constraints,
        //!       which is decided by the internal cooking process of the fabric.
        //!
        //! @param stiffness Stiffness value of vertical constraints.
        //!                  0.0: no vertical constraints.
        //! @param stiffnessMultiplier Scale value for compression and stretch limits.
        //!                            0.0: No horizontal compression and stretch limits applied.
        //!                            1.0: Fully apply compression and stretch limits.
        //! @param compressionLimit Compression limit for vertical constraints.
        //!                         0.0: No compression.
        //! @param stretchLimit Stretch limit for vertical constraints.
        //!                     Reduce stiffness of tether constraints (or increase its scale beyond 1.0) to allow cloth to stretch.
        //!                     0.0: No stretching.
        virtual void SetVerticalPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) = 0;

        //! Configures horizontal constraints parameters.
        //!
        //! @note Applicable only if cloth has horizontal constraints,
        //!       which is decided by the internal cooking process of the fabric.
        //!
        //! @param stiffness Stiffness value of horizontal constraints.
        //!                  0.0: no horizontal constraints.
        //! @param stiffnessMultiplier Scale value for compression and stretch limits.
        //!                            0.0: No horizontal compression and stretch limits applied.
        //!                            1.0: Fully apply compression and stretch limits.
        //! @param compressionLimit Compression limit for horizontal constraints.
        //!                         0.0: No compression.
        //! @param stretchLimit Stretch limit for horizontal constraints.
        //!                     Reduce stiffness of tether constraints (or increase its scale beyond 1.0) to allow cloth to stretch.
        //!                     0.0: No stretching.
        virtual void SetHorizontalPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) = 0;

        //! Configures bending constraints parameters.
        //!
        //! @note Applicable only if cloth has bending constraints,
        //!       which is decided by the internal cooking process of the fabric.
        //!
        //! @param stiffness Stiffness value of bending constraints.
        //!                  0.0: no bending constraints.
        //! @param stiffnessMultiplier Scale value for compression and stretch limits.
        //!                            0.0: No horizontal compression and stretch limits applied.
        //!                            1.0: Fully apply compression and stretch limits.
        //! @param compressionLimit Compression limit for bending constraints.
        //!                         0.0: No compression.
        //! @param stretchLimit Stretch limit for bending constraints.
        //!                     Reduce stiffness of tether constraints (or increase its scale beyond 1.0) to allow cloth to stretch.
        //!                     0.0: No stretching.
        virtual void SetBendingPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) = 0;

        //! Configures shearing constraints parameters.
        //!
        //! @note Applicable only if cloth has shearing constraints,
        //!       which is decided by the internal cooking process of the fabric.
        //!
        //! @param stiffness Stiffness value of shearing constraints.
        //!                  0.0: no shearing constraints.
        //! @param stiffnessMultiplier Scale value for compression and stretch limits.
        //!                            0.0: No horizontal compression and stretch limits applied.
        //!                            1.0: Fully apply compression and stretch limits.
        //! @param compressionLimit Compression limit for shearing constraints.
        //!                         0.0: No compression.
        //! @param stretchLimit Stretch limit for shearing constraints.
        //!                     Reduce stiffness of tether constraints (or increase its scale beyond 1.0) to allow cloth to stretch.
        //!                     0.0: No stretching.
        virtual void SetShearingPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) = 0;

        //! Stiffness for tether constraints.
        //! 0.0: No tether constraints applied.
        //! 1.0: Makes the constraints behave springy.
        //! Stiffness range is [0.0, 1.0].
        //! @note Applicable when cloth has tether constraints,
        //!       that's when fabric data had static particles (inverse mass 0.0) when cooking.
        virtual void SetTetherConstraintStiffness(float stiffness) = 0;

        //! Tether constraints scale.
        //! Scale valid values are >= 0.0.
        //! @note Applicable when cloth has tether constraints,
        //!       that's when fabric data had static particles (inverse mass 0.0) when cooking.
        virtual void SetTetherConstraintScale(float scale) = 0;

        //! Target solver iterations per second.
        //! At least 1 iteration per frame will be solved regardless of the value set.
        //! Frequency valid values are >= 0.0.
        virtual void SetSolverFrequency(float frequency) = 0;

        //! Number of iterations to average delta time factor used for gravity and external acceleration.
        //! Width valid values are > 0.
        virtual void SetAcceleationFilterWidth(AZ::u32 width) = 0;

        //! Set a list of spheres to collide with cloth's particles.
        //! x,y,z represents the position and w the radius of the sphere.
        //!
        //! @note Each cloth can have a maximum of 32 sphere colliders.
        virtual void SetSphereColliders(const AZStd::vector<AZ::Vector4>& spheres) = 0;

        //! Set a list of spheres to collide with cloth's particles (rvalue overload).
        //! x,y,z represents the position and w the radius of the sphere.
        //!
        //! @note Each cloth can have a maximum of 32 sphere colliders.
        virtual void SetSphereColliders(AZStd::vector<AZ::Vector4>&& spheres) = 0;

        //! Set a list of capsules to collide with cloth's particles.
        //! Each capsule is formed by 2 indices to sphere colliders.
        //!
        //! @note Each cloth can have a maximum of 32 capsule colliders.
        //!       In the case that all capsules use unique spheres that maximum
        //!       number would go down to 16, limited by the maximum number of spheres (32).
        virtual void SetCapsuleColliders(const AZStd::vector<AZ::u32>& capsuleIndices) = 0;

        //! Set a list of capsules to collide with cloth's particles (rvalue overload).
        //! Each capsule is formed by 2 indices to sphere colliders.
        //!
        //! @note Each cloth can have a maximum of 32 capsule colliders.
        //!       In the case that all capsules use unique spheres that maximum
        //!       number would go down to 16, limited by the maximum number of spheres (32).
        virtual void SetCapsuleColliders(AZStd::vector<AZ::u32>&& capsuleIndices) = 0;

        //! Sets the motion constraints (positions and radius) for cloth.
        //! Each particle's movement during simulation will be limited to the volume of a sphere.
        //!
        //! @param constraints List of constraints (positions and radius) to set.
        //!
        //! @note Partial set is not allowed, the list must include one constraint per particle.
        virtual void SetMotionConstraints(const AZStd::vector<AZ::Vector4>& constraints) = 0;

        //! Sets the motion constraints (positions and radius) for cloth.
        //! Each particle's movement during simulation will be limited to the volume of a sphere.
        //!
        //! @param constraints List of constraints (positions and radius) to set (rvalue overload).
        //!
        //! @note Partial set is not allowed, the list must include one constraint per particle.
        virtual void SetMotionConstraints(AZStd::vector<AZ::Vector4>&& constraints) = 0;

        //! Clear the list of motion constraints.
        virtual void ClearMotionConstraints() = 0;

        //! Sets the scale to be applied to all motion constraints.
        //! The radius of all motion constraints will be multiplied by the scale.
        //!
        //! @note Internally clamped to avoid negative radius.
        virtual void SetMotionConstraintsScale(float scale) = 0;

        //! Sets the bias to be applied to all motion constraints.
        //! The bias value will be added to the radius of all motion constraints.
        //!
        //! @note Internally clamped to avoid negative radius.
        virtual void SetMotionConstraintsBias(float bias) = 0;

        //! Stiffness for motion constraints.
        //! Stiffness range is [0.0, 1.0].
        virtual void SetMotionConstraintsStiffness(float stiffness) = 0;

        //! Sets the separation constraints (positions and radius) for cloth.
        //! Each particle's movement during simulation will be kept outside the volume of a sphere.
        //!
        //! @param constraints List of constraints (positions and radius) to set.
        //!
        //! @note Partial set is not allowed, the list must include one constraint per particle.
        virtual void SetSeparationConstraints(const AZStd::vector<AZ::Vector4>& constraints) = 0;

        //! Sets the separation constraints (positions and radius) for cloth.
        //! Each particle's movement during simulation will be kept outside the volume of a sphere.
        //!
        //! @param constraints List of constraints (positions and radius) to set (rvalue overload).
        //!
        //! @note Partial set is not allowed, the list must include one constraint per particle.
        virtual void SetSeparationConstraints(AZStd::vector<AZ::Vector4>&& constraints) = 0;

        //! Clear the list of separation constraints.
        virtual void ClearSeparationConstraints() = 0;
    };
} // namespace NvCloth
