/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class AnimGraphManager
    {
    public:
        MOCK_CONST_METHOD1(FindAnimGraphByID, AnimGraph*(uint32));
        MOCK_METHOD2(RecursiveCollectObjectsAffectedBy, void(AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphObject*>& affectedObjects));
    };
} // namespace EMotionFX
