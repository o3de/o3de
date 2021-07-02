/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>

namespace EMotionFX
{
    class SimulatedJoint;

    class SimulatedObject
    {
    public:
        AZ_TYPE_INFO(SimulatedObject, "{8CF0F474-69DC-4DE3-AF19-002F19DA27DB}");
        MOCK_CONST_METHOD1(FindSimulatedJointBySkeletonJointIndex, SimulatedJoint*(AZ::u32));

        MOCK_METHOD1(AddSimulatedJointAndChildren, void(AZ::u32));
        MOCK_METHOD1(AddSimulatedJoint, SimulatedJoint*(AZ::u32));
        MOCK_METHOD1(AddSimulatedJoints, void(AZStd::vector<AZ::u32> joints));

        MOCK_METHOD2(RemoveSimulatedJoint, void(AZ::u32, bool));
        MOCK_METHOD1(RemoveSimulatedJoint, void(AZ::u32));

        MOCK_CONST_METHOD0(GetNumSimulatedJoints, size_t());
        MOCK_CONST_METHOD1(SetSimulatedJoints, void(const AZStd::vector<SimulatedJoint*>& joints));
        MOCK_CONST_METHOD0(GetSimulatedJoints, const AZStd::vector<SimulatedJoint*>&());
        MOCK_CONST_METHOD1(GetSimulatedJoint, SimulatedJoint*(size_t index));

        MOCK_CONST_METHOD0(GetName, const AZStd::string&());
        MOCK_METHOD1(SetName, void(const AZStd::string&));
        MOCK_CONST_METHOD0(GetGravityFactor, float());
        MOCK_METHOD1(SetGravityFactor, void(float));
        MOCK_CONST_METHOD0(GetStiffnessFactor, float());
        MOCK_METHOD1(SetStiffnessFactor, void(float));
        MOCK_CONST_METHOD0(GetDampingFactor, float());
        MOCK_METHOD1(SetDampingFactor, void(float));
        MOCK_CONST_METHOD0(GetColliderTags, AZStd::vector<AZStd::string>&());
        MOCK_METHOD1(SetColliderTags, void(const AZStd::vector<AZStd::string>&));

        MOCK_METHOD0(Clear, void());
        MOCK_METHOD1(InitAfterLoading, void(SimulatedObjectSetup*));
    };
}
