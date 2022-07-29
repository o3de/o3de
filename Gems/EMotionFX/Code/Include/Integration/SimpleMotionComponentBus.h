/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace EMotionFX
{
    class MotionInstance;

    namespace Integration
    {
        class SimpleMotionComponentRequests
            : public AZ::ComponentBus
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual void LoopMotion(bool enable) = 0;
            virtual bool GetLoopMotion() const = 0;
            virtual void RetargetMotion(bool enable) = 0;
            virtual void ReverseMotion(bool enable) = 0;
            virtual void MirrorMotion(bool enable) = 0;
            virtual void SetPlaySpeed(float speed) = 0;
            virtual float GetPlaySpeed() const = 0;
            virtual void PlayTime(float time) = 0;
            virtual float GetPlayTime() const = 0;
            virtual float GetDuration() const = 0;
            virtual void Motion(AZ::Data::AssetId assetId) = 0;
            virtual AZ::Data::AssetId  GetMotion() const = 0;
            virtual void BlendInTime(float time) = 0;
            virtual float GetBlendInTime() const = 0;
            virtual void BlendOutTime(float time) = 0;
            virtual float GetBlendOutTime() const = 0;
            virtual void PlayMotion() = 0;
        };
        using SimpleMotionComponentRequestBus = AZ::EBus<SimpleMotionComponentRequests>;
    }
}
