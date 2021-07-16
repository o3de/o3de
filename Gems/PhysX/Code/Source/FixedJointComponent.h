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
    class FixedJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(FixedJointComponent, "{02E6C633-8F44-4CEE-AE94-DCB06DE36422}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        FixedJointComponent() = default;
        FixedJointComponent(
            const JointComponentConfiguration& configuration, 
            const JointGenericProperties& genericProperties);
        FixedJointComponent(
            const JointComponentConfiguration& configuration, 
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties);
        ~FixedJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;
    };
} // namespace PhysX
