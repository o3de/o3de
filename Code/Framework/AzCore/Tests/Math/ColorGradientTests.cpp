/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/ColorGradient.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <cmath>

using namespace AZ;

namespace UnitTest
{
    // =======================================================================
    // Helpers
    // =======================================================================

    namespace GradientTestHelpers
    {
        ColorGradientMarker MakeColorMarker(const Color& c, float p)
        {
            ColorGradientMarker m;
            m.m_markerColor = c;
            m.m_markerPosition = p;
            return m;
        }

        AlphaGradientMarker MakeAlphaMarker(float a, float p)
        {
            AlphaGradientMarker m;
            m.m_markerAlpha = a;
            m.m_markerPosition = p;
            return m;
        }

        bool GradientsEqual(const ColorGradient& a, const ColorGradient& b)
        {
            constexpr float kTolerance = 0.001f;
            if (a.m_colorSlider.size() != b.m_colorSlider.size()) return false;
            if (a.m_alphaSlider.size() != b.m_alphaSlider.size()) return false;
            for (size_t i = 0; i < a.m_colorSlider.size(); ++i)
            {
                if (std::fabs(a.m_colorSlider[i].m_markerPosition - b.m_colorSlider[i].m_markerPosition) > kTolerance) return false;
                if (!a.m_colorSlider[i].m_markerColor.IsClose(b.m_colorSlider[i].m_markerColor, kTolerance)) return false;
            }
            for (size_t i = 0; i < a.m_alphaSlider.size(); ++i)
            {
                if (std::fabs(a.m_alphaSlider[i].m_markerPosition - b.m_alphaSlider[i].m_markerPosition) > kTolerance) return false;
                if (std::fabs(a.m_alphaSlider[i].m_markerAlpha - b.m_alphaSlider[i].m_markerAlpha) > kTolerance) return false;
            }
            return true;
        }
    }

    // =======================================================================
    // ColorGradient Sampling
    // =======================================================================

    TEST(MATH_ColorGradient, EvaluateColor_EmptyReturnsBlackOpaque)
    {
        // EvaluateColor's contract is "RGB with A=1", so an empty color
        // track samples to opaque black rather than fully transparent zero.
        ColorGradient g;
        EXPECT_THAT(g.EvaluateColor(0.5f), IsClose(Color(0.0f, 0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_ColorGradient, EvaluateColor_SingleStopIsConstant)
    {
        ColorGradient g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.2f, 0.4f, 0.6f, 1.f), 0.5f));
        EXPECT_THAT(g.EvaluateColor(0.f),  IsClose(Color(0.2f, 0.4f, 0.6f, 1.f)));
        EXPECT_THAT(g.EvaluateColor(0.5f), IsClose(Color(0.2f, 0.4f, 0.6f, 1.f)));
        EXPECT_THAT(g.EvaluateColor(1.f),  IsClose(Color(0.2f, 0.4f, 0.6f, 1.f)));
    }

    TEST(MATH_ColorGradient, EvaluateColor_LerpsBetweenTwoStops)
    {
        ColorGradient g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(1.f, 0.f, 0.f, 1.f), 0.f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 0.f, 1.f, 1.f), 1.f));
        const Color mid = g.EvaluateColor(0.5f);
        EXPECT_NEAR(mid.GetR(), 0.5f, 0.001f);
        EXPECT_NEAR(mid.GetG(), 0.0f, 0.001f);
        EXPECT_NEAR(mid.GetB(), 0.5f, 0.001f);
        EXPECT_NEAR(mid.GetA(), 1.0f, 0.001f);
    }

    TEST(MATH_ColorGradient, EvaluateColor_ClampsOutsideRange)
    {
        ColorGradient g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.1f, 0.2f, 0.3f, 1.f), 0.25f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.7f, 0.8f, 0.9f, 1.f), 0.75f));
        EXPECT_THAT(g.EvaluateColor(-1.f), IsClose(Color(0.1f, 0.2f, 0.3f, 1.f)));
        EXPECT_THAT(g.EvaluateColor(2.f),  IsClose(Color(0.7f, 0.8f, 0.9f, 1.f)));
    }

    TEST(MATH_ColorGradient, EvaluateColor_AlwaysOpaque)
    {
        ColorGradient g;
        // Even if user stored a translucent color on the color track, EvaluateColor forces A=1.
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(1.f, 0.5f, 0.25f, 0.0f), 0.5f));
        EXPECT_NEAR(g.EvaluateColor(0.5f).GetA(), 1.0f, 0.001f);
    }

    TEST(MATH_ColorGradient, EvaluateAlpha_EmptyReturnsOne)
    {
        ColorGradient g;
        EXPECT_NEAR(g.EvaluateAlpha(0.5f), 1.f, 0.001f);
    }

    TEST(MATH_ColorGradient, EvaluateAlpha_LerpsBetweenTwoStops)
    {
        ColorGradient g;
        g.m_alphaSlider.push_back(GradientTestHelpers::MakeAlphaMarker(0.f, 0.f));
        g.m_alphaSlider.push_back(GradientTestHelpers::MakeAlphaMarker(1.f, 1.f));
        EXPECT_NEAR(g.EvaluateAlpha(0.25f), 0.25f, 0.001f);
        EXPECT_NEAR(g.EvaluateAlpha(0.75f), 0.75f, 0.001f);
    }

    TEST(MATH_ColorGradient, EvaluateGradient_CombinesColorAndAlpha)
    {
        ColorGradient g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 1.f, 0.f, 1.f), 0.f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 0.f, 0.f, 1.f), 1.f));
        g.m_alphaSlider.push_back(GradientTestHelpers::MakeAlphaMarker(0.2f, 0.f));
        g.m_alphaSlider.push_back(GradientTestHelpers::MakeAlphaMarker(0.8f, 1.f));

        const Color at25 = g.EvaluateGradient(0.25f);
        EXPECT_NEAR(at25.GetR(), 0.0f,  0.001f);
        EXPECT_NEAR(at25.GetG(), 0.75f, 0.001f);
        EXPECT_NEAR(at25.GetB(), 0.0f,  0.001f);
        EXPECT_NEAR(at25.GetA(), 0.35f, 0.001f);

        const Color at75 = g.EvaluateGradient(0.75f);
        EXPECT_NEAR(at75.GetG(), 0.25f, 0.001f);
        EXPECT_NEAR(at75.GetA(), 0.65f, 0.001f);
    }

    TEST(MATH_ColorGradient, SortGradients_HandlesOutOfOrderInsertion)
    {
        ColorGradient g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(1.f, 0.f, 0.f, 1.f), 1.f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 1.f, 0.f, 1.f), 0.5f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 0.f, 1.f, 1.f), 0.f));
        g.SortGradients();
        EXPECT_NEAR(g.m_colorSlider[0].m_markerPosition, 0.f,  0.001f);
        EXPECT_NEAR(g.m_colorSlider[1].m_markerPosition, 0.5f, 0.001f);
        EXPECT_NEAR(g.m_colorSlider[2].m_markerPosition, 1.f,  0.001f);
    }

    TEST(MATH_ColorGradient, SampleWithoutSort_FirstEvaluateAutoSorts)
    {
        ColorGradient g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 0.f, 1.f, 1.f), 1.f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(1.f, 0.f, 0.f, 1.f), 0.f));
        ASSERT_FALSE(g.m_sorted);
        const Color mid = g.EvaluateColor(0.5f);
        EXPECT_TRUE(g.m_sorted);
        EXPECT_NEAR(mid.GetR(), 0.5f, 0.001f);
        EXPECT_NEAR(mid.GetB(), 0.5f, 0.001f);
    }

    // =======================================================================
    // ColorGradientRGB Sampling
    // =======================================================================

    TEST(MATH_ColorGradientRGB, EvaluateColor_BehavesLikeColorTrack)
    {
        ColorGradientRGB g;
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(1.f, 0.f, 0.f, 1.f), 0.f));
        g.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.f, 0.f, 1.f, 1.f), 1.f));
        EXPECT_NEAR(g.EvaluateColor(0.5f).GetR(), 0.5f, 0.001f);
        EXPECT_NEAR(g.EvaluateColor(0.5f).GetA(), 1.0f, 0.001f);
    }

    // =======================================================================
    // Reflection Round-trip
    // =======================================================================

    class ColorGradientReflectionFixture : public ::UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();
            // SerializeContext's constructor invokes MathReflect internally,
            // so all AzCore math types - including ColorGradient and its
            // markers - are already registered.
            m_context = AZStd::make_unique<SerializeContext>();
        }

        void TearDown() override
        {
            m_context.reset();
            UnitTest::LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<SerializeContext> m_context;
    };

    TEST_F(ColorGradientReflectionFixture, SerializeRoundTrip_PreservesMarkers)
    {
        ColorGradient original;
        original.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.1f, 0.2f, 0.3f, 1.f), 0.0f));
        original.m_colorSlider.push_back(GradientTestHelpers::MakeColorMarker(Color(0.7f, 0.8f, 0.9f, 1.f), 1.0f));
        original.m_alphaSlider.push_back(GradientTestHelpers::MakeAlphaMarker(0.25f, 0.0f));
        original.m_alphaSlider.push_back(GradientTestHelpers::MakeAlphaMarker(0.75f, 1.0f));

        AZStd::vector<char> buffer;
        IO::ByteContainerStream<decltype(buffer)> stream(&buffer);
        EXPECT_TRUE(Utils::SaveObjectToStream(stream, DataStream::ST_BINARY, &original, m_context.get()));

        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        ColorGradient restored;
        EXPECT_TRUE(Utils::LoadObjectFromStreamInPlace(stream, restored, m_context.get()));

        EXPECT_TRUE(GradientTestHelpers::GradientsEqual(original, restored));
    }
}
