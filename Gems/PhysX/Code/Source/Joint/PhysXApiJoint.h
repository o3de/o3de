/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzFramework/Physics/Common/PhysicsApiJoint.h>
#include <Joint/Configuration/PhysXJointConfiguration.h>
#include <Joint/PhysXApiJointUtils.h>

namespace PhysX
{
    class PhysXApiJoint
        : public AzPhysics::ApiJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXApiJoint, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysXApiJoint, "{DBE1D185-E318-407D-A5A1-AC1DE7F4A62D}", AzPhysics::ApiJoint);

        PhysXApiJoint(
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXApiJoint() = default;

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
        : public PhysXApiJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXD6Joint, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysXD6Joint, "{144B2FAF-A3EE-4FE1-9328-2C44FE1E3676}", PhysX::PhysXApiJoint);

        PhysXD6Joint(const D6ApiJointLimitConfiguration& configuration,
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
    class PhysXFixedApiJoint : public PhysXApiJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXFixedApiJoint, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysXFixedApiJoint, "{144B2FAF-A3EE-4FE1-9328-2C44FE1E3676}", PhysX::PhysXApiJoint);

        PhysXFixedApiJoint(const FixedApiJointConfiguration& configuration,
            AzPhysics::SceneHandle sceneHandle,
            AzPhysics::SimulatedBodyHandle parentBodyHandle,
            AzPhysics::SimulatedBodyHandle childBodyHandle);

        virtual ~PhysXFixedApiJoint() = default;

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
} // namespace PhysX
