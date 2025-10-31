/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorPhysXJointInterface.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <EditorJointOptimizer.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>

namespace PhysX
{
    AZStd::unique_ptr<AzPhysics::JointConfiguration> PhysXEditorJointHelpersInterface::ComputeOptimalJointLimit(
        const AzPhysics::JointConfiguration* initialConfiguration,
        const AZStd::vector<AZ::Quaternion>& localRotationSamples)
    {
        if (const auto* d6JointLimitConfiguration = azdynamic_cast<const D6JointLimitConfiguration*>(initialConfiguration))
        {
            JointLimitOptimizer::D6JointLimitFitter jointLimitFitter;
            jointLimitFitter.SetChildLocalRotation(d6JointLimitConfiguration->m_childLocalRotation);
            jointLimitFitter.SetLocalRotationSamples(localRotationSamples);
            jointLimitFitter.SetInitialGuess(
                d6JointLimitConfiguration->m_parentLocalRotation, AZ::Constants::QuarterPi, AZ::Constants::QuarterPi);
            D6JointLimitConfiguration fittedLimit = jointLimitFitter.GetFit(d6JointLimitConfiguration->m_childLocalRotation);
            return AZStd::make_unique<D6JointLimitConfiguration>(fittedLimit);
        }

        return nullptr;
    }
} //namespace PhysX
