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

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    namespace JointConstants
    {
        // Setting swing limits to very small values can cause extreme stability problems, so clamp above a small
        // threshold.
        static const float MinSwingLimitDegrees = 1.0f;
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
            PxJointUniquePtr CreatePxD6Joint(const PhysX::D6ApiJointLimitConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);

            PxJointUniquePtr CreatePxFixedJoint(const PhysX::FixedApiJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);

            PxJointUniquePtr CreatePxBallJoint(const PhysX::BallApiJointConfiguration& configuration,
                AzPhysics::SceneHandle sceneHandle,
                AzPhysics::SimulatedBodyHandle parentBodyHandle,
                AzPhysics::SimulatedBodyHandle childBodyHandle);
                
            PxJointUniquePtr CreatePxHingeJoint(const PhysX::HingeApiJointConfiguration& configuration,
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

