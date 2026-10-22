/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;

    // =======================================================================
    // Color Gradient Markers
    // =======================================================================
    // A ColorGradient is defined by two independent tracks sampled over the
    // normalized range [0, 1]: a Color track (RGB) and an Alpha track (scalar
    // opacity). Each track is a list of markers; each marker pins a value at
    // a position in [0, 1].

    //! Pins an RGB color at a normalized position along a ColorGradient.
    //! The alpha channel of m_markerColor is carried for Qt/ColorPicker
    //! convenience but is forced opaque when sampled.
    struct AZCORE_API ColorGradientMarker
    {
        AZ_TYPE_INFO(ColorGradientMarker, "{3F2A8E14-5B9C-4D7E-A123-6E8F9B0C1D2E}");
        AZ_CLASS_ALLOCATOR(ColorGradientMarker, AZ::SystemAllocator);

        AZ::Color m_markerColor = AZ::Color::CreateOne();
        float     m_markerPosition = 0.f;

        static void Reflect(ReflectContext* context);
    };

    //! Pins a scalar opacity at a normalized position along a ColorGradient.
    struct AZCORE_API AlphaGradientMarker
    {
        AZ_TYPE_INFO(AlphaGradientMarker, "{7C4D1F62-9A3B-4E58-B821-5D6A9C0F3E4F}");
        AZ_CLASS_ALLOCATOR(AlphaGradientMarker, AZ::SystemAllocator);

        float m_markerAlpha = 1.f;
        float m_markerPosition = 0.f;

        static void Reflect(ReflectContext* context);
    };

    // =======================================================================
    // Color Gradient (RGBA)
    // =======================================================================
    // Samples a composed RGBA color along [0, 1] from an independent Color
    // track and Alpha track.

    struct AZCORE_API ColorGradient
    {
        AZ_TYPE_INFO(ColorGradient, "{E1B5D837-2C94-4A6F-9D73-1F8A4B7E5C90}");
        AZ_CLASS_ALLOCATOR(ColorGradient, AZ::SystemAllocator);

        AZStd::vector<ColorGradientMarker> m_colorSlider;
        AZStd::vector<AlphaGradientMarker> m_alphaSlider;
        bool m_sorted = false;

        //! Sorts both tracks ascending by m_markerPosition.
        void SortGradients();

        //! Samples the Color track at t in [0, 1]. Returns RGB with A = 1.
        AZ::Color EvaluateColor(float t);

        //! Samples the Alpha track at t in [0, 1].
        float EvaluateAlpha(float t);

        //! Samples both tracks and returns the composed RGBA at t in [0, 1].
        AZ::Color EvaluateGradient(float t);

        static void Reflect(ReflectContext* context);
    };

    // =======================================================================
    // Color Gradient (RGB only)
    // =======================================================================
    // Sibling type for fields that do not need an alpha track. Shares the
    // marker struct and color sampling logic.

    struct AZCORE_API ColorGradientRGB
    {
        AZ_TYPE_INFO(ColorGradientRGB, "{A9D4C672-8B31-4F5E-9A47-2E5B8D1C3F04}");
        AZ_CLASS_ALLOCATOR(ColorGradientRGB, AZ::SystemAllocator);

        AZStd::vector<ColorGradientMarker> m_colorSlider;
        bool m_sorted = false;

        void SortGradients();
        AZ::Color EvaluateColor(float t);

        static void Reflect(ReflectContext* context);
    };
}
