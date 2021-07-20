/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class EMotionFXManager
    {
    public:
        MOCK_CONST_METHOD0(GetActorManager, ActorManager*());
        MOCK_CONST_METHOD0(GetAnimGraphManager, AnimGraphManager*());
    };

    EMotionFXManager& GetEMotionFX()
    {
        static EMotionFXManager manager;
        return manager;
    }

    AnimGraphManager& GetAnimGraphManager()
    {
        return *GetEMotionFX().GetAnimGraphManager();
    }
} // namespace EMotionFX
