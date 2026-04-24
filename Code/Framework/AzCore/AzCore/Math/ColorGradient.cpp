/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/ColorGradient.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

#include <algorithm>

namespace AZ
{
    // =======================================================================
    // Internal Sampling Helpers
    // =======================================================================
    // Single-responsibility samplers. Both assume the input vector is sorted
    // ascending by markerPosition.

    namespace Internal
    {
        static AZ::Color SampleColorTrack(float t, const AZStd::vector<ColorGradientMarker>& slider)
        {
            if (slider.empty())
            {
                return AZ::Color::CreateZero();
            }

            if (t <= slider.front().markerPosition) { return slider.front().markerColor; }
            if (t >= slider.back().markerPosition)  { return slider.back().markerColor; }

            for (size_t i = 0; i + 1 < slider.size(); ++i)
            {
                const auto& a = slider[i];
                const auto& b = slider[i + 1];
                if (t >= a.markerPosition && t <= b.markerPosition)
                {
                    const float span = b.markerPosition - a.markerPosition;
                    const float localT = (span > 0.f) ? (t - a.markerPosition) / span : 0.f;
                    return a.markerColor.Lerp(b.markerColor, localT);
                }
            }

            return slider.back().markerColor;
        }

        static float SampleAlphaTrack(float t, const AZStd::vector<AlphaGradientMarker>& slider)
        {
            if (slider.empty())
            {
                return 1.f;
            }

            if (t <= slider.front().markerPosition) { return slider.front().markerAlpha; }
            if (t >= slider.back().markerPosition)  { return slider.back().markerAlpha; }

            for (size_t i = 0; i + 1 < slider.size(); ++i)
            {
                const auto& a = slider[i];
                const auto& b = slider[i + 1];
                if (t >= a.markerPosition && t <= b.markerPosition)
                {
                    const float span = b.markerPosition - a.markerPosition;
                    const float localT = (span > 0.f) ? (t - a.markerPosition) / span : 0.f;
                    return a.markerAlpha + (b.markerAlpha - a.markerAlpha) * localT;
                }
            }

            return slider.back().markerAlpha;
        }

        static AZ::Color ForceOpaque(const AZ::Color& in)
        {
            return AZ::Color(in.GetR(), in.GetG(), in.GetB(), 1.f);
        }

        template <typename MarkerT>
        static void SortByPosition(AZStd::vector<MarkerT>& v)
        {
            if (v.empty()) { return; }
            std::sort(
                v.begin(), v.end(),
                [](const MarkerT& a, const MarkerT& b) { return a.markerPosition < b.markerPosition; });
        }
    }

    // =======================================================================
    // ColorGradientMarker Reflection
    // =======================================================================

    void ColorGradientMarker::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<ColorGradientMarker>()
                ->Version(1)
                ->Field("markerColor", &ColorGradientMarker::markerColor)
                ->Field("markerPosition", &ColorGradientMarker::markerPosition)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<ColorGradientMarker>("ColorGradientMarker", "Pins an RGB color at a normalized position along the gradient.")
                    ->DataElement(nullptr, &ColorGradientMarker::markerColor, "Color", "The RGB color at this marker.")
                    ->DataElement(nullptr, &ColorGradientMarker::markerPosition, "Position", "Normalized position in [0,1].")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ;
            }
        }

        if (auto* bc = azrtti_cast<BehaviorContext*>(context))
        {
            bc->Class<ColorGradientMarker>("ColorGradientMarker")
                ->Attribute(AZ::Script::Attributes::Category, "Color Gradient")
                ->Property("markerColor", BehaviorValueProperty(&ColorGradientMarker::markerColor))
                ->Property("markerPosition", BehaviorValueProperty(&ColorGradientMarker::markerPosition))
                ;
        }
    }

    // =======================================================================
    // AlphaGradientMarker Reflection
    // =======================================================================

    void AlphaGradientMarker::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<AlphaGradientMarker>()
                ->Version(1)
                ->Field("markerAlpha", &AlphaGradientMarker::markerAlpha)
                ->Field("markerPosition", &AlphaGradientMarker::markerPosition)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<AlphaGradientMarker>("AlphaGradientMarker", "Pins a scalar opacity at a normalized position along the gradient.")
                    ->DataElement(nullptr, &AlphaGradientMarker::markerAlpha, "Alpha", "Opacity at this marker, in [0,1].")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(nullptr, &AlphaGradientMarker::markerPosition, "Position", "Normalized position in [0,1].")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ;
            }
        }

        if (auto* bc = azrtti_cast<BehaviorContext*>(context))
        {
            bc->Class<AlphaGradientMarker>("AlphaGradientMarker")
                ->Attribute(AZ::Script::Attributes::Category, "Color Gradient")
                ->Property("markerAlpha", BehaviorValueProperty(&AlphaGradientMarker::markerAlpha))
                ->Property("markerPosition", BehaviorValueProperty(&AlphaGradientMarker::markerPosition))
                ;
        }
    }

    // =======================================================================
    // ColorGradient (RGBA)
    // =======================================================================

    void ColorGradient::SortGradients()
    {
        Internal::SortByPosition(colorSlider);
        Internal::SortByPosition(alphaSlider);
        sorted = true;
    }

    AZ::Color ColorGradient::EvaluateColor(float t)
    {
        if (!sorted) { SortGradients(); }
        return Internal::ForceOpaque(Internal::SampleColorTrack(t, colorSlider));
    }

    float ColorGradient::EvaluateAlpha(float t)
    {
        if (!sorted) { SortGradients(); }
        return Internal::SampleAlphaTrack(t, alphaSlider);
    }

    AZ::Color ColorGradient::EvaluateGradient(float t)
    {
        if (!sorted) { SortGradients(); }
        const AZ::Color rgb = Internal::SampleColorTrack(t, colorSlider);
        const float a = Internal::SampleAlphaTrack(t, alphaSlider);
        return AZ::Color(rgb.GetR(), rgb.GetG(), rgb.GetB(), a);
    }

    void ColorGradient::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<ColorGradient>()
                ->Version(1)
                ->Field("colorSlider", &ColorGradient::colorSlider)
                ->Field("alphaSlider", &ColorGradient::alphaSlider)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<ColorGradient>("Color Gradient", "RGB and alpha gradient sampled by a normalized position in [0,1].")
                    ->DataElement(nullptr, &ColorGradient::colorSlider, "Color Slider", "Color track markers.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                    ->DataElement(nullptr, &ColorGradient::alphaSlider, "Alpha Slider", "Alpha track markers.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                    ;
            }
        }

        if (auto* bc = azrtti_cast<BehaviorContext*>(context))
        {
            bc->Class<ColorGradient>("ColorGradient")
                ->Attribute(AZ::Script::Attributes::Category, "Color Gradient")
                ->Method("EvaluateColor", &ColorGradient::EvaluateColor)
                ->Method("EvaluateAlpha", &ColorGradient::EvaluateAlpha)
                ->Method("EvaluateGradient", &ColorGradient::EvaluateGradient)
                ->Method("SortGradients", &ColorGradient::SortGradients)
                ;
        }
    }

    // =======================================================================
    // ColorGradientRGB (RGB only)
    // =======================================================================

    void ColorGradientRGB::SortGradients()
    {
        Internal::SortByPosition(colorSlider);
        sorted = true;
    }

    AZ::Color ColorGradientRGB::EvaluateColor(float t)
    {
        if (!sorted) { SortGradients(); }
        return Internal::ForceOpaque(Internal::SampleColorTrack(t, colorSlider));
    }

    void ColorGradientRGB::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<ColorGradientRGB>()
                ->Version(1)
                ->Field("colorSlider", &ColorGradientRGB::colorSlider)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<ColorGradientRGB>("Color Gradient (RGB)", "RGB-only gradient sampled by a normalized position in [0,1].")
                    ->DataElement(nullptr, &ColorGradientRGB::colorSlider, "Color Slider", "Color track markers.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                    ;
            }
        }

        if (auto* bc = azrtti_cast<BehaviorContext*>(context))
        {
            bc->Class<ColorGradientRGB>("ColorGradientRGB")
                ->Attribute(AZ::Script::Attributes::Category, "Color Gradient")
                ->Method("EvaluateColor", &ColorGradientRGB::EvaluateColor)
                ->Method("SortGradients", &ColorGradientRGB::SortGradients)
                ;
        }
    }
}
