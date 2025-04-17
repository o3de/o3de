/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <StarsAsset.h>
#include <Stars/StarsFeatureProcessorInterface.h>

namespace AZ::Render
{
    inline constexpr AZ::TypeId StarsComponentTypeId{ "{A0DC17A5-9494-47EF-AD6D-BC563739A02B}" };
    inline constexpr AZ::TypeId EditorStarsComponentTypeId{ "{460B0A4E-6A6F-4AFF-9668-4B5AA2F33C09}" };

    class StarsComponentConfig final
        : public ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(StarsComponentConfig, SystemAllocator)
        AZ_RTTI(AZ::Render::StarsComponentConfig, "{10E6A838-3A66-4518-BF53-FCA8325C4759}", AZ::ComponentConfig);

        static void Reflect(ReflectContext* context);

        float m_exposure = StarsDefaultExposure;
        float m_radiusFactor = StarsDefaultRadiusFactor;
        float m_twinkleRate = StarsDefaultTwinkleRate;

        AZ::Data::Asset<StarsAsset> m_starsAsset;
    };
}
