/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>

namespace AzPhysics
{
    struct JointConfiguration;
} // namespace AzPhysics

namespace PhysX
{
    class PhysXEditorJointHelpersInterface
        : public AZ::Interface<AzPhysics::EditorJointHelpersInterface>::Registrar
    {
    public:
        AZ_RTTI(PhysX::PhysXEditorJointHelpersInterface, "{ACEA4FB4-045F-45FB-819E-B4C86A63ED6A}", AzPhysics::EditorJointHelpersInterface);

        AZStd::unique_ptr<AzPhysics::JointConfiguration> ComputeOptimalJointLimit(
            const AzPhysics::JointConfiguration* initialConfiguration,
            const AZStd::vector<AZ::Quaternion>& localRotationSamples) override;
    };
} // namespace PhysX
