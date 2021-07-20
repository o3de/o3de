#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <SceneAPIExt/Rules/IActorScaleRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class ActorScaleRule
                : public IActorScaleRule
            {
            public:
                AZ_RTTI(ActorScaleRule, "{29A7688B-45DA-449E-9862-8ADD99645F69}", IActorScaleRule);
                AZ_CLASS_ALLOCATOR_DECL

                ActorScaleRule();
                ~ActorScaleRule() override = default;

                void SetScaleFactor(float value);

                // IActorScaleRule overrides
                float GetScaleFactor() const override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                float m_scaleFactor;
            };
        }  // Rule
    }  // Pipeline
}  // EMotionFX
