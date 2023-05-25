/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/ring_buffer.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <EMotionFX/Source/RagdollVelocityEvaluators.h>


namespace EMotionFX
{
    class RagdollVelocityEvaluator
    {
    public:
        AZ_RTTI(RagdollVelocityEvaluator, "{F16A725B-F4D9-4A15-B9EB-B2D47EA993A6}")
        AZ_CLASS_ALLOCATOR_DECL

        RagdollVelocityEvaluator() = default;
        virtual ~RagdollVelocityEvaluator() = default;

        /// Calculate the linear and angular velocities for all nodes in the ragdoll based on the last, the current poses and the time delta.
        virtual void CalculateVelocities(Physics::RagdollState& outRagdollPose, const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta);

        // This is called each frame even when the ragdoll is inactive.
        virtual void Update(const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta) = 0;

        /// This is called when the ragdoll gets activated.
        virtual void CalculateInitialVelocities(Physics::RagdollState& outRagdollPose) = 0;
    };

    /**
     * Calculate the initial velocities by averaging historic ragdoll poses based on their time deltas.
     * This is using a moving average for non-equally spaced samples where keep the historic poses in the window
     * up to date and store them in a ring buffer. As the samples's time delta vary, the minimum time window for
     * the moving average can be controlled.
     */
    class MovingAverageVelocityEvaluator
        : public RagdollVelocityEvaluator
    {
    public:
        AZ_RTTI(MovingAverageVelocityEvaluator, "{A7A84B76-C642-4CE2-B141-A2455A3F06E8}", RagdollVelocityEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        MovingAverageVelocityEvaluator();
        ~MovingAverageVelocityEvaluator() = default;

        void Update(const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta) override;
        void CalculateInitialVelocities(Physics::RagdollState& outRagdollPose) override;

    private:
        float CalcTotalHistoryTimeInterval() const;
        void CalcHistoryPoseIndex(size_t& outIndex, float& outTimeDelta);

        AZStd::ring_buffer<AZStd::pair<Physics::RagdollState, float>>   m_poseHistory;      /**< Ragdoll history pose ring buffer. Oldest pose in front (older poses at lower indices) and the youngest at the back (higher indices). */
        float                                                           m_minTimeWindow;    /**< The minimum time window in seconds for the moving average. */
    };

    /**
     * The running average velocity evaluator calculates the velocity based on the last and current pose each frame and equally
     * weights it with the running average. This result is a smaller memory footprint as there is no need to store the pose history 
     * but also ignores the time deltas and exponentially smoothed out older velocities.
     */
    class RunningAverageVelocityEvaluator
        : public RagdollVelocityEvaluator
    {
    public:
        AZ_RTTI(RunningAverageVelocityEvaluator, "{A74CEDF2-92A5-4788-9711-8D1DBA0C9D04}", RagdollVelocityEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        RunningAverageVelocityEvaluator() = default;
        ~RunningAverageVelocityEvaluator() = default;

        void Update(const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta) override;
        void CalculateInitialVelocities(Physics::RagdollState& outRagdollPose) override;

    private:
        Physics::RagdollState m_running;
        Physics::RagdollState m_last;
    };
} // namespace EMotionFX
