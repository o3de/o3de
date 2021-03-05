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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    class SmoothStep final
    {
    public:
        AZ_CLASS_ALLOCATOR(SmoothStep, AZ::SystemAllocator, 0);
        AZ_RTTI(SmoothStep, "{F392F061-BF40-43C5-89F6-7323D6EF11F4}");
        static void Reflect(AZ::ReflectContext* context);

        inline float GetSmoothedValue(float inputValue) const;

        float m_falloffMidpoint = 0.5f;
        float m_falloffRange = 0.5f;
        float m_falloffStrength = 0.25f;
    };

    inline float SmoothStep::GetSmoothedValue(float inputValue) const
    {
        float output = 0.0f;

        const float value = AZ::GetClamp(inputValue, 0.0f, 1.0f);
        const float valueFalloffRange = AZ::GetClamp(m_falloffRange, 0.0f, 1.0f);
        const float valueFalloffStrength = AZ::GetClamp(m_falloffStrength, 0.0f, 1.0f);

        float min = m_falloffMidpoint - m_falloffRange / 2.0f;
        float max = m_falloffMidpoint + m_falloffRange / 2.0f;

        float result1 = GetRatio(min, min + valueFalloffStrength, value);
        result1 = GetSmoothStep(result1);
        float result2 = GetRatio(max - valueFalloffStrength, max, value);
        result2 = GetSmoothStep(result2);

        output = result1 * (1.0f - result2);

        return output;
    }
}