/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <PhysX/Joint/PhysXJointRequestsBus.h>
#include <Source/JointComponent.h>

namespace PhysX
{
    //! Provides runtime support for prismatic joints.
    //! Prismatic joints allow no rotation, but allow sliding along a direction aligned with the x-axis of both bodies'
    //! joint frames.
    class PrismaticJointComponent
        : public JointComponent
        , public JointInterfaceRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PrismaticJointComponent, "{9B34CA1B-C063-4D42-A15B-CE6CD7C828DC}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        PrismaticJointComponent() = default;
        PrismaticJointComponent(
            const JointComponentConfiguration& configuration,
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties,
            const JointMotorProperties& motorProperties);
        ~PrismaticJointComponent() = default;

        ///////////////////////////////////////////////////////////////////////////////////
        // JointInterfaceRequestBus::Handler overrides
        float GetPosition() const override;
        float GetVelocity() const override;
        AZ::Transform GetTransform() const override;
        void SetVelocity(float velocity) override;
        void SetMaximumForce(float force) override;
        AZStd::pair<float, float> GetLimits() const override;
        ///////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        ///////////////////////////////////////////////////////////////////////////////////

    protected:
        // JointComponent
        void InitNativeJoint() override;

        // get Physx native type
        physx::PxD6Joint* GetPhysXD6Joint() const;

    private:
        mutable physx::PxD6Joint* m_native{ nullptr };
    };
} // namespace PhysX
