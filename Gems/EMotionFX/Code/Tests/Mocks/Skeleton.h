/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class Skeleton
    {
    public:
        MOCK_CONST_METHOD1(GetNode, Node*(uint32 index));
        MOCK_CONST_METHOD1(FindNodeByName, Node*(const char* name));
        MOCK_CONST_METHOD0(GetNumNodes, uint32());
    };
} // namespace EMotionFX
