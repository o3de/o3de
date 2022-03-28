/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FeaturePosition.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>
#include <FeatureSchemaDefault.h>

namespace EMotionFX::MotionMatching
{
    void DefaultFeatureSchema(FeatureSchema& featureSchema, DefaultFeatureSchemaInitSettings settings)
    {
        featureSchema.Clear();
        const AZStd::string rootJointName = settings.m_rootJointName;

        //----------------------------------------------------------------------------------------------------------
        // Past and future root trajectory
        FeatureTrajectory* rootTrajectory = aznew FeatureTrajectory();
        rootTrajectory->SetJointName(rootJointName);
        rootTrajectory->SetRelativeToJointName(rootJointName);
        rootTrajectory->SetDebugDrawColor(AZ::Color::CreateFromRgba(157,78,221,255));
        rootTrajectory->SetDebugDrawEnabled(true);
        featureSchema.AddFeature(rootTrajectory);

        //----------------------------------------------------------------------------------------------------------
        // Left foot position
        FeaturePosition* leftFootPosition = aznew FeaturePosition();
        leftFootPosition->SetName("Left Foot Position");
        leftFootPosition->SetJointName(settings.m_leftFootJointName);
        leftFootPosition->SetRelativeToJointName(rootJointName);
        leftFootPosition->SetDebugDrawColor(AZ::Color::CreateFromRgba(255,173,173,255));
        leftFootPosition->SetDebugDrawEnabled(true);
        featureSchema.AddFeature(leftFootPosition);

        //----------------------------------------------------------------------------------------------------------
        // Right foot position
        FeaturePosition* rightFootPosition = aznew FeaturePosition();
        rightFootPosition->SetName("Right Foot Position");
        rightFootPosition->SetJointName(settings.m_rightFootJointName);
        rightFootPosition->SetRelativeToJointName(rootJointName);
        rightFootPosition->SetDebugDrawColor(AZ::Color::CreateFromRgba(253,255,182,255));
        rightFootPosition->SetDebugDrawEnabled(true);
        featureSchema.AddFeature(rightFootPosition);

        //----------------------------------------------------------------------------------------------------------
        // Left foot velocity
        FeatureVelocity* leftFootVelocity = aznew FeatureVelocity();
        leftFootVelocity->SetName("Left Foot Velocity");
        leftFootVelocity->SetJointName(settings.m_leftFootJointName);
        leftFootVelocity->SetRelativeToJointName(rootJointName);
        leftFootVelocity->SetDebugDrawColor(AZ::Color::CreateFromRgba(155,246,255,255));
        leftFootVelocity->SetDebugDrawEnabled(true);
        leftFootVelocity->SetCostFactor(0.75f);
        featureSchema.AddFeature(leftFootVelocity);

        //----------------------------------------------------------------------------------------------------------
        // Right foot velocity
        FeatureVelocity* rightFootVelocity = aznew FeatureVelocity();
        rightFootVelocity->SetName("Right Foot Velocity");
        rightFootVelocity->SetJointName(settings.m_rightFootJointName);
        rightFootVelocity->SetRelativeToJointName(rootJointName);
        rightFootVelocity->SetDebugDrawColor(AZ::Color::CreateFromRgba(189,178,255,255));
        rightFootVelocity->SetDebugDrawEnabled(true);
        rightFootVelocity->SetCostFactor(0.75f);
        featureSchema.AddFeature(rightFootVelocity);

        //----------------------------------------------------------------------------------------------------------
        // Pelvis velocity
        FeatureVelocity* pelvisVelocity = aznew FeatureVelocity();
        pelvisVelocity->SetName("Pelvis Velocity");
        pelvisVelocity->SetJointName(settings.m_pelvisJointName);
        pelvisVelocity->SetRelativeToJointName(rootJointName);
        pelvisVelocity->SetDebugDrawColor(AZ::Color::CreateFromRgba(185,255,175,255));
        pelvisVelocity->SetDebugDrawEnabled(true);
        featureSchema.AddFeature(pelvisVelocity);
    }
} // namespace EMotionFX::MotionMatching
