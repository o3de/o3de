/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <Joint/PhysXJointUtils.h>

namespace PhysX
{
    class PhysXJoint
        : public AzPhysics::Joint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXJoint, AZ::SystemAllocator);
        AZ_RTTI(PhysXJoint, "{DBE1D185-E318-407D-A5A1-AC1DE7F4A62D}", AzPhysics::Joint);

        PhysXJoint(
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXJoint() = default;

        AzPhysics::SimulatedBodyHandle GetParentBodyHandle() const override;
        AzPhysics::SimulatedBodyHandle GetChildBodyHandle() const override;
        void SetParentBody(AzPhysics::SimulatedBodyHandle parentBody) override;
        void SetChildBody(AzPhysics::SimulatedBodyHandle childBody) override;
        void* GetNativePointer() const override;

    protected:
        bool SetPxActors();

        Utils::PxJointUniquePtr m_pxJoint;
        AzPhysics::SceneHandle m_sceneHandle;
        AzPhysics::SimulatedBodyHandle m_parentBodyHandle;
        AzPhysics::SimulatedBodyHandle m_childBodyHandle;
        AZStd::string m_name;
    };

    class PhysXD6Joint
        : public PhysXJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXD6Joint, AZ::SystemAllocator);
        AZ_RTTI(PhysXD6Joint, "{144B2FAF-A3EE-4FE1-9328-2C44FE1E3676}", PhysX::PhysXJoint);

        PhysXD6Joint(const D6JointLimitConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXD6Joint() = default;

        AZ::Crc32 GetNativeType() const override;
        void GenerateJointLimitVisualizationData(
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) override;
    };

    //! A fixed joint locks 2 bodies relative to one another on all axes of freedom.
    class PhysXFixedJoint : public PhysXJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXFixedJoint, AZ::SystemAllocator);
        AZ_RTTI(PhysXFixedJoint, "{B821D6D8-7B41-479D-9325-F9BC9754C5F8}", PhysX::PhysXJoint);

        PhysXFixedJoint(const FixedJointConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXFixedJoint() = default;

        AZ::Crc32 GetNativeType() const override;
    };

    //! A ball joint locks 2 bodies relative to one another except about the y and z axes of the joint between them.
    class PhysXBallJoint : public PhysXJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXBallJoint, AZ::SystemAllocator);
        AZ_RTTI(PhysXBallJoint, "{9494CE43-3AE2-40AB-ADF7-FDC5F8B0F15A}", PhysX::PhysXJoint);

        PhysXBallJoint(const BallJointConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXBallJoint() = default;

        AZ::Crc32 GetNativeType() const override;
    };

    //! A hinge joint locks 2 bodies relative to one another except about the x-axis of the joint between them.
    class PhysXHingeJoint : public PhysXJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXHingeJoint, AZ::SystemAllocator);
        AZ_RTTI(PhysXHingeJoint, "{9C5B955C-6C80-45FA-855D-DDA449C85313}", PhysX::PhysXJoint);

        PhysXHingeJoint(const HingeJointConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXHingeJoint() = default;

        AZ::Crc32 GetNativeType() const override;
    };

    class PhysXPrismaticJoint : public PhysXJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXPrismaticJoint, AZ::SystemAllocator);
        AZ_RTTI(PhysXPrismaticJoint, "{CEE6A6DF-FDE1-4E30-9EE2-631C7561C1C7}", PhysX::PhysXJoint);

        PhysXPrismaticJoint(const PrismaticJointConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXPrismaticJoint() = default;

        AZ::Crc32 GetNativeType() const override;
    };
} // namespace PhysX
