/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ISkinRule.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class SkinRule
                : public DataTypes::ISkinRule
            {
            public:
                AZ_RTTI(SkinRule, "{B26E7FC9-86A1-4711-8415-8BE4861C08BA}", ISkinRule);
                AZ_CLASS_ALLOCATOR_DECL

                SkinRule();
                ~SkinRule() override = default;

                AZ::u32 GetMaxWeightsPerVertex() const override;
                float GetWeightThreshold() const override;

                static void Reflect(AZ::ReflectContext* context);

            protected:
                AZ::u32 m_maxWeightsPerVertex;
                float m_weightThreshold;
            };
        }
    }
}
