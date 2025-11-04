/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace EMotionFX
{
    class SimulatedObject;

    class SimulatedJoint
    {
    public:
        AZ_CLASS_ALLOCATOR(SimulatedJoint, AZ::SystemAllocator)
        AZ_TYPE_INFO(SimulatedJoint, "{4434F175-2A60-4F54-9A7D-243DAAD8C811}");

        using AutoExcludeMode = ::EMotionFX::SimulatedJoint::AutoExcludeMode;

        SimulatedJoint() = default;
        SimulatedJoint([[maybe_unused]] const SimulatedJoint& simulatedJoint) {}

        MOCK_METHOD1(SetSimulatedObject, void (SimulatedObject* object));
        MOCK_METHOD1(SetSkeletonJointIndex, void (size_t jointIndex));
        MOCK_METHOD1(SetConeAngleLimit, void (float degrees));
        MOCK_METHOD1(SetMass, void (float mass));
        MOCK_METHOD1(SetStiffness, void (float stiffness));
        MOCK_METHOD1(SetDamping, void (float damping));
        MOCK_METHOD1(SetGravityFactor, void (float factor));
        MOCK_METHOD1(SetFriction, void (float friction));
        MOCK_METHOD1(SetPinned, void (bool pinned));
        MOCK_METHOD1(InitAfterLoading, bool (SimulatedObject* object));

        MOCK_CONST_METHOD0(GetSkeletonJointIndex, size_t());
        MOCK_CONST_METHOD0(GetConeAngleLimit, float());
        MOCK_CONST_METHOD0(GetMass, float());
        MOCK_CONST_METHOD0(GetStiffness, float());
        MOCK_CONST_METHOD0(GetDamping, float());
        MOCK_CONST_METHOD0(GetGravityFactor, float());
        MOCK_CONST_METHOD0(GetFriction, float());
        MOCK_CONST_METHOD0(IsPinned, bool());

        MOCK_CONST_METHOD0(GetColliderExclusionTags, AZStd::vector<AZStd::string>& ());
        MOCK_METHOD1(SetColliderExclusionTags, void(const AZStd::vector<AZStd::string>&));

        MOCK_CONST_METHOD0(GetAutoExcludeMode, AutoExcludeMode());
        MOCK_METHOD1(SetAutoExcludeMode, void(AutoExcludeMode mode));

        MOCK_CONST_METHOD0(IsGeometricAutoExclusion, bool());
        MOCK_METHOD1(SetGeometricAutoExclusion, void(bool enabled));

        MOCK_CONST_METHOD0(CalculateNumChildSimulatedJoints, size_t());
        MOCK_CONST_METHOD1(FindChildSimulatedJoint, SimulatedJoint*(size_t childIndex));
    };
} // namespace EMotionFX
