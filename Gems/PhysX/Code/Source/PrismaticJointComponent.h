/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Source/JointComponent.h>

namespace PhysX
{
    //! Provides runtime support for prismatic joints.
    //! Prismatic joints allow no rotation, but allow sliding along a direction aligned with the x-axis of both bodies'
    //! joint frames.
    class PrismaticJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(PrismaticJointComponent, "{9B34CA1B-C063-4D42-A15B-CE6CD7C828DC}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        PrismaticJointComponent() = default;
        PrismaticJointComponent(
            const JointComponentConfiguration& configuration,
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties);
        ~PrismaticJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;
    };
} // namespace PhysX
