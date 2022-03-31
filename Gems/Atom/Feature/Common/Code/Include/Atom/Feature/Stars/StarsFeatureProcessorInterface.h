/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    static const float StarsDefaultIntensityFactor = 3.0f;
    static const float StarsDefaultRadiusFactor = 7.0f;

    class StarsFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::StarsFeatureProcessorInterface, "{7ECE8A5E-366B-4942-B6F9-370DC6017927}"); 

        struct StarVertex 
        {
            AZStd::array<float, 3> m_position;
            uint32_t m_color;
        };

        virtual void Enable(bool enable) = 0;
        virtual bool IsEnabled() = 0;
        virtual void SetStars(const AZStd::vector<StarVertex>& starVertexData) = 0;
        virtual void SetIntensityFactor(float intensityFactor) = 0;
        virtual void SetRadiusFactor(float radiusFactor) = 0;
        virtual void SetOrientation(AZ::Quaternion orientation) = 0;
    };
}
