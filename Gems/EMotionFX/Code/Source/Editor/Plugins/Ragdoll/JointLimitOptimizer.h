/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>

namespace EMotionFX
{
    inline const int JointLimitOptimizerTotalSamples = 1000; //!< Approximate number of joint poses to sample from available motion sets.
    inline const int JointLimitOptimizerMaxSamplesPerMotion = 50; //!< Upper limit on number of samples from a single motion.

    void OptimizeJointLimits(const PhysicsSetupManipulatorData& physicsSetupManipulatorData);
} // namespace EMotionFX
