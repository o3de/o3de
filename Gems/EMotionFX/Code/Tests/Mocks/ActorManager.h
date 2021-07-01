/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class ActorManager
    {
    public:
        MOCK_CONST_METHOD1(FindActorByID, Actor*(AZ::u32));
    };
} // namespace EMotionFX
