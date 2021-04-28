#pragma once

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
