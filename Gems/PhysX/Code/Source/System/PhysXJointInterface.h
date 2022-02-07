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
}

namespace PhysX
{
    class PhysXJointHelpersInterface
        : public AZ::Interface<AzPhysics::JointHelpersInterface>::Registrar
    {
    public:
        AZ_RTTI(PhysX::PhysXJointHelpersInterface, "{48AC5137-2226-4C57-8E4C-FCF3C1965252}", AzPhysics::JointHelpersInterface);

        const AZStd::vector<AZ::TypeId> GetSupportedJointTypeIds() const override;
        AZStd::optional<const AZ::TypeId> GetSupportedJointTypeId(AzPhysics::JointType typeEnum) const override;

        AZStd::unique_ptr<AzPhysics::JointConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations) override;

        void GenerateJointLimitVisualizationData(
            const AzPhysics::JointConfiguration& configuration,
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
