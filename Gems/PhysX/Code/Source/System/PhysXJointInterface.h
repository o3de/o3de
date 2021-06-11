/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
 * a third party where indicated.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Common/PhysicsApiJoint.h>

namespace AzPhysics
{
    struct ApiJointConfiguration;
}

namespace PhysX
{
    class PhysXJointHelpersInterface
        : public AZ::Interface<AzPhysics::JointHelpersInterface>::Registrar
    {
    public:
        AZ_RTTI(PhysX::PhysXJointHelpersInterface, "{48AC5137-2226-4C57-8E4C-FCF3C1965252}", AzPhysics::JointHelpersInterface);

        AZStd::unique_ptr<AzPhysics::ApiJointConfiguration> ComputeInitialJointLimitConfiguration(
            const AzPhysics::JointTypes& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations) override;

        void GenerateJointLimitVisualizationData(
            const AzPhysics::ApiJointConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) override;
    };
}
