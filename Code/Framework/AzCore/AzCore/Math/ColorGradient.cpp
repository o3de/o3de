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
    // One templated sampler covers both tracks. Caller supplies the empty
    // fallback, a value accessor that pulls the marker's value field, and a
    // lerp callable. Assumes the input vector is sorted ascending by
    // m_markerPosition.

    namespace Internal
    {
        template <typename MarkerT, typename ValueT, typename ValueOf, typename Lerp>
        static ValueT SampleTrack(
            float t,
            const AZStd::vector<MarkerT>& slider,
            ValueT emptyFallback,
            ValueOf valueOf,
            Lerp lerp)
        {
            if (slider.empty())
            {
                return emptyFallback;
            }

            if (t <= slider.front().m_markerPosition) { return valueOf(slider.front()); }
            if (t >= slider.back().m_markerPosition)  { return valueOf(slider.back()); }

            for (size_t i = 0; i + 1 < slider.size(); ++i)
            {
                const auto& a = slider[i];
                const auto& b = slider[i + 1];
                if (t >= a.m_markerPosition && t <= b.m_markerPosition)
                {
                    const float span = b.m_markerPosition - a.m_markerPosition;
                    const float localT = (span > 0.f) ? (t - a.m_markerPosition) / span : 0.f;
                    return lerp(valueOf(a), valueOf(b), localT);
                }
            }

            return valueOf(slider.back());
        }

        static AZ::Color SampleColorTrack(float t, const AZStd::vector<ColorGradientMarker>& slider)
        {
            return SampleTrack<ColorGradientMarker, AZ::Color>(
                t, slider, AZ::Color::CreateZero(),
                [](const ColorGradientMarker& m) { return m.m_markerColor; },
                [](const AZ::Color& a, const AZ::Color& b, float l) { return a.Lerp(b, l); });
        }

        static float SampleAlphaTrack(float t, const AZStd::vector<AlphaGradientMarker>& slider)
        {
            return SampleTrack<AlphaGradientMarker, float>(
                t, slider, 1.f,
                [](const AlphaGradientMarker& m) { return m.m_markerAlpha; },
                [](float a, float b, float l) { return a + (b - a) * l; });
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
                [](const MarkerT& a, const MarkerT& b) { return a.m_markerPosition < b.m_markerPosition; });
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
                ->Field("markerColor", &ColorGradientMarker::m_markerColor)
                ->Field("markerPosition", &ColorGradientMarker::m_markerPosition)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<ColorGradientMarker>("ColorGradientMarker", "Pins an RGB color at a normalized position along the gradient.")
                    ->DataElement(nullptr, &ColorGradientMarker::m_markerColor, "Color", "The RGB color at this marker.")
                    ->DataElement(nullptr, &ColorGradientMarker::m_markerPosition, "Position", "Normalized position in [0,1].")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ;
            }
        }

        if (auto* bc = azrtti_cast<BehaviorContext*>(context))
        {
            bc->Class<ColorGradientMarker>("ColorGradientMarker")
                ->Attribute(AZ::Script::Attributes::Category, "Color Gradient")
                ->Property("markerColor", BehaviorValueProperty(&ColorGradientMarker::m_markerColor))
                ->Property("markerPosition", BehaviorValueProperty(&ColorGradientMarker::m_markerPosition))
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
                ->Field("markerAlpha", &AlphaGradientMarker::m_markerAlpha)
                ->Field("markerPosition", &AlphaGradientMarker::m_markerPosition)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<AlphaGradientMarker>("AlphaGradientMarker", "Pins a scalar opacity at a normalized position along the gradient.")
                    ->DataElement(nullptr, &AlphaGradientMarker::m_markerAlpha, "Alpha", "Opacity at this marker, in [0,1].")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(nullptr, &AlphaGradientMarker::m_markerPosition, "Position", "Normalized position in [0,1].")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ;
            }
        }

        if (auto* bc = azrtti_cast<BehaviorContext*>(context))
        {
            bc->Class<AlphaGradientMarker>("AlphaGradientMarker")
                ->Attribute(AZ::Script::Attributes::Category, "Color Gradient")
                ->Property("markerAlpha", BehaviorValueProperty(&AlphaGradientMarker::m_markerAlpha))
                ->Property("markerPosition", BehaviorValueProperty(&AlphaGradientMarker::m_markerPosition))
                ;
        }
    }

    // =======================================================================
    // ColorGradient (RGBA)
    // =======================================================================

    void ColorGradient::SortGradients()
    {
        Internal::SortByPosition(m_colorSlider);
        Internal::SortByPosition(m_alphaSlider);
        m_sorted = true;
    }

    AZ::Color ColorGradient::EvaluateColor(float t)
    {
        if (!m_sorted) { SortGradients(); }
        return Internal::ForceOpaque(Internal::SampleColorTrack(t, m_colorSlider));
    }

    float ColorGradient::EvaluateAlpha(float t)
    {
        if (!m_sorted) { SortGradients(); }
        return Internal::SampleAlphaTrack(t, m_alphaSlider);
    }

    AZ::Color ColorGradient::EvaluateGradient(float t)
    {
        if (!m_sorted) { SortGradients(); }
        const AZ::Color rgb = Internal::SampleColorTrack(t, m_colorSlider);
        const float a = Internal::SampleAlphaTrack(t, m_alphaSlider);
        return AZ::Color(rgb.GetR(), rgb.GetG(), rgb.GetB(), a);
    }

    void ColorGradient::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<ColorGradient>()
                ->Version(1)
                ->Field("colorSlider", &ColorGradient::m_colorSlider)
                ->Field("alphaSlider", &ColorGradient::m_alphaSlider)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<ColorGradient>("Color Gradient", "RGB and alpha gradient sampled by a normalized position in [0,1].")
                    ->DataElement(nullptr, &ColorGradient::m_colorSlider, "Color Slider", "Color track markers.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                    ->DataElement(nullptr, &ColorGradient::m_alphaSlider, "Alpha Slider", "Alpha track markers.")
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
        Internal::SortByPosition(m_colorSlider);
        m_sorted = true;
    }

    AZ::Color ColorGradientRGB::EvaluateColor(float t)
    {
        if (!m_sorted) { SortGradients(); }
        return Internal::ForceOpaque(Internal::SampleColorTrack(t, m_colorSlider));
    }

    void ColorGradientRGB::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<ColorGradientRGB>()
                ->Version(1)
                ->Field("colorSlider", &ColorGradientRGB::m_colorSlider)
                ;

            if (EditContext* ec = sc->GetEditContext())
            {
                ec->Class<ColorGradientRGB>("Color Gradient (RGB)", "RGB-only gradient sampled by a normalized position in [0,1].")
                    ->DataElement(nullptr, &ColorGradientRGB::m_colorSlider, "Color Slider", "Color track markers.")
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
