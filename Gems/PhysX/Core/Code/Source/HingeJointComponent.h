/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <PhysX/Joint/PhysXJointRequestsBus.h>
#include <Source/JointComponent.h>

namespace PhysX
{
    class HingeJointComponent
        : public JointComponent
        , public JointRequestBus::Handler
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

        // JointRequestBus::Handler overrides ...
        float GetPosition() const override;
        float GetVelocity() const override;
        AZ::Transform GetTransform() const override;
        void SetVelocity(float velocity) override;
        void SetMaximumForce(float force) override;
        AZStd::pair<float, float> GetLimits() const override;

    protected:
        // JointComponent overrides ...
        void InitNativeJoint() override;
        void DeinitNativeJoint() override;

    private:
        void CachePhysXNativeRevoluteJoint();

        physx::PxRevoluteJoint* m_nativeJoint{ nullptr };
    };
} // namespace PhysX
