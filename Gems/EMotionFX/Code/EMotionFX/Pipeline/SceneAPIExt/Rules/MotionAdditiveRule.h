#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                AZ::u64 m_sampleFrameIndex;
            };
        } // Rule
    } // Pipeline
} // EMotionFX
