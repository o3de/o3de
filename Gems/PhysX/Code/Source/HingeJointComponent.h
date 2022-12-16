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
#include <PhysX/Joint/PhysXJointBus.h>

namespace PhysX
{
    class HingeJointComponent
        : public JointComponent
        , public JointInterfaceRequestBus::Handler
    {
    public:
        AZ_COMPONENT(HingeJointComponent, "{A5CA0031-72E4-4908-B764-EDECD3091882}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        HingeJointComponent() = default;
        HingeJointComponent(
            const JointComponentConfiguration& configuration, 
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties,
            const JointMotorProperties& motorProperties = JointMotorProperties());
        ~HingeJointComponent() = default;

        // JointInterfaceRequestBus::Handler
        float GetPosition() override;
        float GetVelocity() override;
        AZ::Transform GetTransform() override;

        // JointMotorRequestBus ::Handler
        void SetVelocity(float velocity) override;
        void SetMaximumForce(float force) override;

        AZStd::pair<float, float> GetLimits();
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    protected:
        // JointComponent
        void InitNativeJoint() override;
    };
} // namespace PhysX
