/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class Skeleton;
    class SimulatedObjectSetup;

    class Actor
    {
    public:
        MOCK_CONST_METHOD0(GetSkeleton, Skeleton*());
        MOCK_CONST_METHOD0(GetID, uint32());
        MOCK_CONST_METHOD0(GetSimulatedObjectSetup, const AZStd::shared_ptr<SimulatedObjectSetup>&());
        MOCK_CONST_METHOD0(GetDirtyFlag, bool());
        MOCK_CONST_METHOD1(SetDirtyFlag, void(bool));
    };
} // namespace EMotionFX
