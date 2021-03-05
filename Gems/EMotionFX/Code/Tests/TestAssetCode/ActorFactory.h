/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/false, /*generateOBBs=*/false, /*convertUnitType=*/false);
            return actor;
        }
    };
} // namespace EMotionFX
