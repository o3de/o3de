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
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

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
            class MotionAdditiveRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(MotionAdditiveRule, "{FCC4EFC5-73CB-4C4F-8CFA-47ECC57BECAB}", IRule);
                AZ_CLASS_ALLOCATOR_DECL

                MotionAdditiveRule();
                ~MotionAdditiveRule() override = default;

                size_t GetSampleFrameIndex() const;
                void SetSampleFrameIndex(size_t index);

                static void Reflect(AZ::ReflectContext* context);

            protected:
                size_t m_sampleFrameIndex;
            };
        } // Rule
    } // Pipeline
} // EMotionFX