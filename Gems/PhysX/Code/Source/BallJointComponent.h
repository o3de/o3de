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
    class BallJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(BallJointComponent, "{914036AC-195E-4517-B58E-D29E42A560B9}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        BallJointComponent() = default;
        BallJointComponent(
            const JointComponentConfiguration& configuration, 
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties);
        ~BallJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;
    };
} // namespace PhysX
