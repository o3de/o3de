/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <PxPhysicsAPI.h>

#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace PhysX
{
    struct D6JointLimitConfiguration;
    struct FixedJointConfiguration;
    struct BallJointConfiguration;
    struct HingeJointConfiguration;
    struct PrismaticJointConfiguration;

    namespace JointConstants
    {
        // Setting joint limits to very small values can cause extreme stability problems, so clamp above a small
        // threshold.
        static const float MinSwingLimitDegrees = 1.0f;
        // Minimum range between lower and upper twist limits.
        static const float MinTwistLimitRangeDegrees = 1.0f;
    } // namespace JointConstants

    namespace Utils
    {
        using PxJointUniquePtr = AZStd::unique_ptr<physx::PxJoint, AZStd::function<void(physx::PxJoint*)>>;

        bool IsAtLeastOneDynamic(AzPhysics::SimulatedBody* body0, AzPhysics::SimulatedBody* body1);

        physx::PxRigidActor* GetPxRigidActor(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle worldBodyHandle);
        AzPhysics::SimulatedBody* GetSimulatedBodyFromHandle(
            AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle);

        namespace PxJointFactories
        {
            PxJointUniquePtr CreatePxD6Joint(const PhysX::D6JointLimitConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);

            PxJointUniquePtr CreatePxFixedJoint(const PhysX::FixedJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);

            PxJointUniquePtr CreatePxBallJoint(const PhysX::BallJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);

            PxJointUniquePtr CreatePxHingeJoint(const PhysX::HingeJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);

            PxJointUniquePtr CreatePxPrismaticJoint(const PhysX::PrismaticJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);
        } // namespace PxActorFactories

        namespace Joints
        {
            bool IsD6SwingValid(float swingAngleY, float swingAngleZ, float swingLimitY, float swingLimitZ);

            void AppendD6SwingConeToLineBuffer(
                const AZ::Quaternion& parentLocalRotation,
                float swingAngleY,
                float swingAngleZ,
                float swingLimitY,
                float swingLimitZ,
                float scale,
                AZ::u32 angularSubdivisions,
                AZ::u32 radialSubdivisions,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut);

            void AppendD6TwistArcToLineBuffer(
                const AZ::Quaternion& parentLocalRotation,
                float twistAngle,
                float twistLimitLower,
                float twistLimitUpper,
                float scale,
                AZ::u32 angularSubdivisions,
                AZ::u32 radialSubdivisions,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut);

            void AppendD6CurrentTwistToLineBuffer(
                const AZ::Quaternion& parentLocalRotation,
                float twistAngle,
                float twistLimitLower,
                float twistLimitUpper,
                float scale,
                AZStd::vector<AZ::Vector3>& lineBufferOut,
                AZStd::vector<bool>& lineValidityBufferOut);
        } // namespace Joints
    } // namespace Utils
} // namespace PhysX

