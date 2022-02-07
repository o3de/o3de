/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace EMotionFX
{
    class ActorFactory
    {
    public:
        template<class ActorT, typename... Args>
        static AZStd::unique_ptr<ActorT> CreateAndInit(Args&&... args)
        {
            AZStd::unique_ptr<ActorT> actor = AZStd::make_unique<ActorT>(AZStd::forward<Args>(args)...);
            actor->SetID(0);
            actor->GetSkeleton()->UpdateNodeIndexValues(0);
            actor->ResizeTransformData();
            actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/false, /*convertUnitType=*/false);
            return actor;
        }
    };
} // namespace EMotionFX
