/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class SimulatedObjectSetup
    {
    public:
        MOCK_METHOD0(AddSimulatedObject, void());
        MOCK_METHOD1(AddSimulatedObject, void(const AZStd::string&));
        MOCK_METHOD1(InsertSimulatedObjectAt, SimulatedObject*(size_t));
        MOCK_METHOD1(RemoveSimulatedObject, void(size_t));
        MOCK_CONST_METHOD0(GetNumSimulatedObjects, size_t());
        MOCK_CONST_METHOD1(GetSimulatedObject, SimulatedObject*(size_t));
        MOCK_CONST_METHOD1(FindSimulatedObjectByJoint, SimulatedObject*(const SimulatedJoint*));
        MOCK_CONST_METHOD1(GetSimulatedObjectIndex, AZ::Outcome<size_t>(const SimulatedObject*));
        MOCK_CONST_METHOD0(GetSimulatedObjects, const AZStd::vector<SimulatedObject*>&());
        MOCK_CONST_METHOD2(IsSimulatedObjectNameUnique, bool(const AZStd::string&, const SimulatedObject*));
    };
}
