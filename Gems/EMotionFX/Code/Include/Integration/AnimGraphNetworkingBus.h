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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace EMotionFX
{
    using NodeIndexContainer = AZStd::vector<AZ::u32>;
    using MotionPlayTimeEntry = AZStd::pair<AZ::u32, float>;
    using MotionNodePlaytimeContainer = AZStd::vector<MotionPlayTimeEntry>; 

    // Networking request bus
    class AnimGraphComponentNetworkRequests
        : public AZ::ComponentBus
    {
    public:

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool IsAssetReady() const = 0;
        virtual bool HasSnapshot() const = 0;
        virtual void CreateSnapshot(bool isAuthoritative) = 0;
        virtual void SetActiveStates(const NodeIndexContainer& activeStates) = 0;
        virtual const NodeIndexContainer& GetActiveStates() const = 0;
        virtual void SetMotionPlaytimes(const MotionNodePlaytimeContainer& motionNodePlaytimes) = 0;
        virtual const MotionNodePlaytimeContainer& GetMotionPlaytimes() const = 0;
        virtual void UpdateActorExternal(float deltatime) = 0;
        virtual void SetNetworkRandomSeed(AZ::u64 seed) = 0;
        virtual AZ::u64 GetNetworkRandomSeed() const = 0;
    };

    using AnimGraphComponentNetworkRequestBus = AZ::EBus<AnimGraphComponentNetworkRequests>;
}