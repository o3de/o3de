/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Source/JointComponent.h>

namespace PhysX
{
    class HingeJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(HingeJointComponent, "{A5CA0031-72E4-4908-B764-EDECD3091882}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        HingeJointComponent() = default;
        explicit HingeJointComponent(const GenericJointConfiguration& config
            , const GenericJointLimitsConfiguration& angularLimit);
        ~HingeJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;

    private:
        void InitAngularLimits();
    };
} // namespace PhysX
