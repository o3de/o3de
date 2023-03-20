/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzCore/Casting/numeric_cast.h>

static const qreal g_colorChannelTolerance = 0.0000005;

class ColorChangedSignalSpy : public QObject
{
public:
    ColorChangedSignalSpy(AzQtComponents::Internal::ColorController* controller)
    {
        connect(controller, &AzQtComponents::Internal::ColorController::colorChanged, this, [this]() {
            m_count++;
        });
    }

    int count() const { return m_count; }

private:
    int m_count = 0;
};

namespace
{

    struct RGB
    {
        // 0..255
        double r;
        double g;
        double b;
    };

    struct HSL
    {
        // 0.0..360.0
        double h;
        // 0.0..100.0
        double s;
        double l;
    };

    struct HSV
    {
        // 0.0..360.0
        double h;
        // 0.0..100.0
        double s;
        double v;
    };

    static void TestFromRGB(const RGB& rgb, const HSL& hsl, const HSV& hsv)
    {
        using namespace AZ;
        using namespace AzQtComponents;
        using namespace AzQtComponents::Internal;

        // Default the controller to something non-zero, so that when we set it to zero, it actually changes properly
        // and emits signals. Otherwise, the 0 case won't emit any signals because there wasn't a change
        const AZ::Color nonZeroDefaultRGB(0.5f, 0.5f, 0.5f, 0.5f);

        // Set RGB, check others
        ColorController controller;
        controller.setColor(nonZeroDefaultRGB);

        ColorChangedSignalSpy spy(&controller);

        controller.setRed(aznumeric_caster(rgb.r / 255.0));
        controller.setGreen(aznumeric_caster(rgb.g / 255.0));
        controller.setBlue(aznumeric_caster(rgb.b / 255.0));

        EXPECT_NEAR(controller.hslHue(), aznumeric_caster(hsl.h / 360.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.hslSaturation(), aznumeric_caster(hsl.s / 100.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.lightness(), aznumeric_caster(hsl.l / 100.0), g_colorChannelTolerance);

        EXPECT_NEAR(controller.hsvHue(), aznumeric_caster(hsv.h / 360.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.hsvSaturation(), aznumeric_caster(hsv.s / 100.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.value(), aznumeric_caster(hsv.v / 100.0), g_colorChannelTolerance);

        EXPECT_EQ(spy.count(), 3);
    }

    static void TestFromHSL(const RGB& rgb, const HSL& hsl, const HSV& hsv)
    {
        using namespace AZ;
        using namespace AzQtComponents;
        using namespace AzQtComponents::Internal;

        // Default the controller to something non-zero, so that when we set it to zero, it actually changes properly
        // and emits signals. Otherwise, the 0 case won't emit any signals because there wasn't a change
        const HSL nonZeroDefaultHSL{ 180.0, 50.0, 50.0 };

        // Set HSL, check others
        ColorController controller;
        controller.setHslHue(aznumeric_caster(nonZeroDefaultHSL.h / 360.0));
        controller.setHslSaturation(aznumeric_caster(nonZeroDefaultHSL.s / 100.0));
        controller.setLightness(aznumeric_caster(nonZeroDefaultHSL.l / 100.0));

        ColorChangedSignalSpy spy(&controller);

        controller.setHslHue(aznumeric_caster(hsl.h / 360.0));
        controller.setHslSaturation(aznumeric_caster(hsl.s / 100.0));
        controller.setLightness(aznumeric_caster(hsl.l / 100.0));

        EXPECT_NEAR(controller.red(), aznumeric_caster(rgb.r / 255.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.green(), aznumeric_caster(rgb.g / 255.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.blue(), aznumeric_caster(rgb.b / 255.0), g_colorChannelTolerance);

        EXPECT_NEAR(controller.hsvHue(), aznumeric_caster(hsv.h / 360.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.hsvSaturation(), aznumeric_caster(hsv.s / 100.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.value(), aznumeric_caster(hsv.v / 100.0), g_colorChannelTolerance);

        EXPECT_EQ(spy.count(), 3);
    }

    static void TestFromHSV(const RGB& rgb, const HSL& hsl, const HSV& hsv)
    {
        using namespace AZ;
        using namespace AzQtComponents;
        using namespace AzQtComponents::Internal;

        // Default the controller to something non-zero, so that when we set it to zero, it actually changes properly
        // and emits signals. Otherwise, the 0 case won't emit any signals because there wasn't a change
        const HSV nonZeroDefaultHSV{ 180.0, 50.0, 50.0 };

        // Set HSV, check others
        ColorController controller;
        controller.setHsvHue(aznumeric_caster(nonZeroDefaultHSV.h / 360.0));
        controller.setHsvSaturation(aznumeric_caster(nonZeroDefaultHSV.s / 100.0));
        controller.setValue(aznumeric_caster(nonZeroDefaultHSV.v / 100.0));

        ColorChangedSignalSpy spy(&controller);

        controller.setHsvHue(aznumeric_caster(hsv.h / 360.0));
        controller.setHsvSaturation(aznumeric_caster(hsv.s / 100.0));
        controller.setValue(aznumeric_caster(hsv.v / 100.0));

        EXPECT_NEAR(controller.red(), aznumeric_caster(rgb.r / 255.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.green(), aznumeric_caster(rgb.g / 255.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.blue(), aznumeric_caster(rgb.b / 255.0), g_colorChannelTolerance);

        EXPECT_NEAR(controller.hslHue(), aznumeric_caster(hsl.h / 360.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.hslSaturation(), aznumeric_caster(hsl.s / 100.0), g_colorChannelTolerance);
        EXPECT_NEAR(controller.lightness(), aznumeric_caster(hsl.l / 100.0), g_colorChannelTolerance);

        EXPECT_EQ(spy.count(), 3);
    }

    static void TestConversions(const RGB& rgb, const HSL& hsl, const HSV& hsv)
    {
        TestFromRGB(rgb, hsl, hsv);
        TestFromHSL(rgb, hsl, hsv);
        TestFromHSV(rgb, hsl, hsv);
    }
}

TEST(AzQtComponents, ColorConversionsTestAllZeros)
{
    TestConversions({ 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 });
}

// The ColorConversionsTestColorWheelSection are intended to test that conversions with the hue values
// set along all 6 sections of the color wheel work properly

TEST(AzQtComponents, ColorConversionsTestColorWheelSection0)
{
    // Test with the hue set between 0 - 60 degrees
    TestConversions({ 255.0, 140.0, 45.0 }, { 27.14285910129547, 100.0, 58.82352941176471 }, { 27.14285910129547, 82.35294117647058, 100.0 });
}

TEST(AzQtComponents, ColorConversionsTestColorWheelSection1)
{
    // Test with the hue set between 60 - 120 degrees
    TestConversions({ 67, 110, 58 }, { 109.61538076400757, 30.95238208770752, 32.941177487373352 }, { 109.61538076400757, 47.272726893424988, 43.137255311012268 });
}

TEST(AzQtComponents, ColorConversionsTestColorWheelSection2)
{
    // Test with the hue set between 120 - 180 degrees
    TestConversions({ 163, 231, 245 }, { 190.24389266967773, 80.392158031463623, 80.000001192092896 }, { 190.24389266967773, 33.469384908676147, 96.078431606292725 });
}

TEST(AzQtComponents, ColorConversionsTestColorWheelSection3)
{
    // Test with the hue set between 180 - 240 degrees
    TestConversions({ 145.0, 146.0, 147.0 }, { 209.99990344015873, 0.9174282126790563, 57.25490324637469 }, { 209.99990344015873, 1.3605442176870763, 57.64705882352941 });
}

TEST(AzQtComponents, ColorConversionsTestColorWheelSection4)
{
    // Test with the hue set between 240 - 300 degrees
    TestConversions({ 63, 59, 110 }, { 244.70588922500610, 30.177515745162964, 33.137255907058716 }, { 244.70588922500610, 46.363636851310730, 43.137255311012268 });
}

TEST(AzQtComponents, ColorConversionsTestColorWheelSection5)
{
    // Test with the hue set between 300 - 360 degrees
    TestConversions({ 110, 59, 97 }, { 315.29412031173706, 30.177515745162964, 33.137255907058716 }, { 315.29412031173706, 46.363636851310730, 43.137255311012268 });
}

TEST(AzQtComponents, ColorConversionsMoreTest)
{
    TestConversions({ 230.0, 217.0, 203.0 }, { 31.111114427385832, 35.06493767873301, 84.90196107649335 }, { 31.111114427385832, 11.739130434782608, 90.19607843137255 });
}

TEST(AzQtComponents, ColorConversionsTestHueMaximum_HSL)
{
    using namespace AZ;
    using namespace AzQtComponents;
    using namespace AzQtComponents::Internal;

    // Verify that setting the hue to the maximum updates all channels properly
    // when working with HSL color space
    {
        ColorController controller;
        ColorChangedSignalSpy spy(&controller);

        controller.setColor(AZ::Color(0.f, 1.f, 0.f, 1.f));

        EXPECT_NEAR(controller.red(), 0.f, g_colorChannelTolerance);
        EXPECT_NEAR(controller.green(), 1.f, g_colorChannelTolerance);
        EXPECT_NEAR(controller.blue(), 0.f, g_colorChannelTolerance);
        EXPECT_NEAR(controller.alpha(), 1.f, g_colorChannelTolerance);

        {
            AZ::Color previousColor = controller.color();
            controller.setHslHue(1.f);
            EXPECT_NE(controller.color(), previousColor);

            previousColor = controller.color();
            controller.setColor(AZ::Color(.5f, 0.f, 0.f, 1.f));
            EXPECT_NE(controller.color(), previousColor);
        }

        EXPECT_EQ(spy.count(), 3);
    }
}

TEST(AzQtComponents, ColorConversionsTestHueMaximum_HSV)
{
    using namespace AZ;
    using namespace AzQtComponents;
    using namespace AzQtComponents::Internal;

    // Verify that setting the hue to the maximum updates all channels properly
    // when working with HSV color space
    {
        ColorController controller;
        ColorChangedSignalSpy spy(&controller);

        controller.setColor(AZ::Color(0.f, 1.f, 0.f, 1.f));

        EXPECT_NEAR(controller.red(), 0.f, g_colorChannelTolerance);
        EXPECT_NEAR(controller.green(), 1.f, g_colorChannelTolerance);
        EXPECT_NEAR(controller.blue(), 0.f, g_colorChannelTolerance);
        EXPECT_NEAR(controller.alpha(), 1.f, g_colorChannelTolerance);

        {
            AZ::Color previousColor = controller.color();
            controller.setHsvHue(1.f);
            EXPECT_NE(controller.color(), previousColor);

            previousColor = controller.color();
            controller.setColor(AZ::Color(.5f, 0.f, 0.f, 1.f));
            EXPECT_NE(controller.color(), previousColor);
        }

        EXPECT_EQ(spy.count(), 3);
    }
}
