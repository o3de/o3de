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

#include <Allocators.h>
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <FeatureDirection.h>
#include <Feature.h>
#include <LocomotionBehavior.h>
#include <MotionMatchEventData.h>
#include <MotionMatchSystem.h>
#include <FeaturePosition.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        void MotionMatchSystem::Reflect(AZ::ReflectContext* context)
        {
            // Event data.
            MotionMatchEventData::Reflect(context);

            // Behaviors.
            BehaviorInstance::Reflect(context);
            Behavior::Reflect(context);
            LocomotionBehavior::Reflect(context);

            // Frame Data.
            Feature::Reflect(context);
            FeaturePosition::Reflect(context);
            FeatureTrajectory::Reflect(context);
            FeatureVelocity::Reflect(context);
            FeatureDirection::Reflect(context);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
