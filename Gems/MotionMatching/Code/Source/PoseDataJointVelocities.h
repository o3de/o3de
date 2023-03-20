/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/PoseData.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace EMotionFX::MotionMatching
{
    //! Extends a given pose with joint-relative linear and angular velocities.
    class EMFX_API PoseDataJointVelocities
        : public PoseData
    {
    public:
        AZ_RTTI(PoseDataJointVelocities, "{9C082B82-7225-4550-A52C-C920CCC2482C}", PoseData)
        AZ_CLASS_ALLOCATOR_DECL

        PoseDataJointVelocities();
        ~PoseDataJointVelocities();

        void Clear();

        void LinkToActorInstance(const ActorInstance* actorInstance) override;
        void LinkToActor(const Actor* actor) override;

        //! Zero all linear and angular velocities.
        void Reset() override;

        void CopyFrom(const PoseData* from) override;
        void Blend(const Pose* destPose, float weight) override;

        //! Calculate velocities for all joints in the pose.
        //! @param[in] actorInstance The actor instance to use the skeleton and bind pose from.
        //! @param[in] posePool Calculating velocities will require to sample the motion across a small window of time. The pose pool is used for storing temporary poses.
        //!                     Note that the pose pool is not thread-safe.
        //! @param[in] motion The source motion to use to calculate the velocities.
        //! @param[in] requestedSampleTime The point in time in the motion to calculate the velocities for.
        //! @param[in] relativeToJointIndex Calculate velocities relative to a given joint transform.
        void CalculateVelocity(const ActorInstance* actorInstance, AnimGraphPosePool& posePool, Motion* motion, float requestedSampleTime, size_t relativeToJointIndex);

        void DebugDraw(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Color& color) const override;

        AZStd::vector<AZ::Vector3>& GetVelocities()                         { return m_velocities; }
        const AZStd::vector<AZ::Vector3>& GetVelocities() const             { return m_velocities; }
        const AZ::Vector3& GetVelocity(size_t jointIndex)                   { return m_velocities[jointIndex]; }

        AZStd::vector<AZ::Vector3>& GetAngularVelocities()                  { return m_angularVelocities; }
        const AZStd::vector<AZ::Vector3>& GetAngularVelocities() const      { return m_angularVelocities; }
        const AZ::Vector3& GetAngularVelocity(size_t jointIndex)            { return m_angularVelocities[jointIndex]; }

        static void Reflect(AZ::ReflectContext* context);

        void SetRelativeToJointIndex(size_t relativeToJointIndex);

    private:
        AZStd::vector<AZ::Vector3> m_velocities;
        AZStd::vector<AZ::Vector3> m_angularVelocities;
        size_t m_relativeToJointIndex = InvalidIndex;
    };
} // namespace EMotionFX::MotionMatching
