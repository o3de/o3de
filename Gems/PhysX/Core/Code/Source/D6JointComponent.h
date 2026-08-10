/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <PhysX/Joint/PhysXJointRequestsBus.h>
#include <Source/JointComponent.h>

namespace PhysX
{
    constexpr AZStd::string_view D6JointFlag = "/Amazon/Physics/EnableD6Joints";

    //! Helper function for checking whether feature flag for in progress PhysX D6 joint are enabled.
    //! PhysX D6 is advanced Joint type, and should be used for specific applications.
    inline bool D6JointsEnabled()
    {
        bool reducedCoordinateArticulationsEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(reducedCoordinateArticulationsEnabled, D6JointFlag);
        }

        return reducedCoordinateArticulationsEnabled;
    }

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(D6JointAxis, uint8_t, Free, Limited, Locked);

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(D6JointDriveType, uint8_t, None, Position, Velocity);

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(D6JointDriveFlag, uint8_t, Force, Acceleration);

    AZ_TYPE_INFO_SPECIALIZE(D6JointDriveType, "{2C4F5641-E543-21F1-C669-1911311D3B77}");
    AZ_TYPE_INFO_SPECIALIZE(D6JointDriveFlag, "{DA6A6960-94BE-11F1-B55A-0800200C9A66}");
    AZ_TYPE_INFO_SPECIALIZE(D6JointAxis, "{1B3F4530-D432-11F0-B558-0800200C9A66}");

    struct D6JointDrive
    {
        AZ_CLASS_ALLOCATOR(D6JointDrive, AZ::SystemAllocator);
        AZ_TYPE_INFO(D6JointDrive, "{3D4E5F60-A1B2-2C3D-4E5F-6A7B8C9D0E1F}");
        static void Reflect(AZ::ReflectContext* context);

        D6JointDriveType m_driveType = D6JointDriveType::None;
        D6JointDriveFlag m_driveFlag = D6JointDriveFlag::Force;
        float m_stiffness = 0.0f;
        float m_damping = 0.0f;
        float m_forceLimit = AZ::Constants::FloatMax;

        bool IsActive() const
        {
            return m_driveType != D6JointDriveType::None;
        }
    };

    struct D6JointComponentConfiguration
    {
        AZ_CLASS_ALLOCATOR(D6JointComponentConfiguration, AZ::SystemAllocator);
        AZ_TYPE_INFO(D6JointComponentConfiguration, "{C8A3B7E2-9F4D-4A21-8F6A-2D3E5C7A9B1F}");
        static void Reflect(AZ::ReflectContext* context);

        D6JointComponentConfiguration() = default;

        D6JointAxis m_motionX = D6JointAxis::Locked;
        D6JointAxis m_motionY = D6JointAxis::Locked;
        D6JointAxis m_motionZ = D6JointAxis::Locked;
        D6JointAxis m_motionTwist = D6JointAxis::Locked;
        D6JointAxis m_motionSwing1 = D6JointAxis::Locked;
        D6JointAxis m_motionSwing2 = D6JointAxis::Locked;

        AZStd::pair<float, float> m_motionLimitsX{ -1.f, 1.f };
        AZStd::pair<float, float> m_motionLimitsY{ -1.f, 1.f };
        AZStd::pair<float, float> m_motionLimitsZ{ -1.f, 1.f };
        AZStd::pair<float, float> m_motionLimitsTwist{ -AZ::Constants::Pi, AZ::Constants::Pi };
        AZStd::pair<float, float> m_motionLimitsCone{ -AZ::Constants::Pi / 4, AZ::Constants::Pi / 4 };

        D6JointDrive m_driveX;
        D6JointDrive m_driveY;
        D6JointDrive m_driveZ;
        D6JointDrive m_driveTwist;
        D6JointDrive m_driveSwing;
        D6JointDrive m_driveSlerp;

        bool IsXLimited() const
        {
            return m_motionX == D6JointAxis::Limited;
        }
        bool IsYLimited() const
        {
            return m_motionY == D6JointAxis::Limited;
        }
        bool IsZLimited() const
        {
            return m_motionZ == D6JointAxis::Limited;
        }
        bool IsTwistLimited() const
        {
            return m_motionTwist == D6JointAxis::Limited;
        }
        bool IsSwing1Limited() const
        {
            return m_motionSwing1 == D6JointAxis::Limited;
        }
        bool IsSwing2Limited() const
        {
            return m_motionSwing2 == D6JointAxis::Limited;
        }
    };

    //! Runtime D6 joint component.
    //! Provides a 6 degree of freedom joint constraint between two entities.
    //! D6 joints allow full control over all linear and angular motion, making them
    //! suitable for complex constraints like ragdoll joints, sliding doors with rotation, etc.
    class D6JointComponent
        : public JointComponent
        , public JointRequestBus::Handler
    {
    public:
        AZ_COMPONENT(D6JointComponent, "{F3A4C5D6-E7F8-9A0B-1C2D-3E4F5A6B7C8D}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        D6JointComponent() = default;
        D6JointComponent(
            const JointComponentConfiguration& configuration,
            const JointGenericProperties& genericProperties,
            const D6JointComponentConfiguration& d6Configuration);
        ~D6JointComponent() = default;
    protected:
        // JointRequestBus::Handler overrides
        float GetPosition() const override;
        float GetVelocity() const override;
        AZStd::pair<AZ::Vector3, AZ::Vector3> GetVelocityGeneral() const override;
        AZ::Transform GetTransform() const override;
        void SetVelocity(float velocity) override;
        void SetVelocityGeneral(const AZ::Vector3& linear, const AZ::Vector3& angular) override;
        void SetMaximumForce(float force) override;
        void SetMaximumForceAxis(int id, float force) override;
        AZStd::pair<float, float> GetLimits() const override;
        AZStd::pair<float, float> GetLimitsAxis(int id) const override;
        void* GetNativeJointPointer() const override
        {
            return m_nativeD6Joint;
        }

        // JointComponent overrides
        void InitNativeJoint() override;
        void DeinitNativeJoint() override;

    private:
        physx::PxD6Joint* m_nativeD6Joint{ nullptr };
        D6JointComponentConfiguration m_d6Configuration;
    };

} // namespace PhysX
