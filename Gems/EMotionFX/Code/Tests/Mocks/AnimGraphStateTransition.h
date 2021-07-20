/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class AnimGraphStateTransition
    {
    public:
        MOCK_CONST_METHOD0(GetAnimGraph, AnimGraph*());
        MOCK_CONST_METHOD0(GetNumConditions, size_t());
        MOCK_CONST_METHOD1(GetCondition, AnimGraphTransitionCondition*(size_t));
    };
} // namespace EMotionFX
