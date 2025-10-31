/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Color, Construction)
    {
        // Default constructor
        Color colorDefault;
        colorDefault.SetR(1.0f);
        EXPECT_NEAR(colorDefault.GetR(), 1.0f, Constants::Tolerance);

        // Vector4 constructor
        const Vector4 vectorColor(0.5f, 0.6f, 0.7f, 1.0f);
        const Color colorFromVector(vectorColor);
        EXPECT_THAT(colorFromVector.GetAsVector4(), IsClose(vectorColor));

        // Single float constructor
        const Color colorFromFloat(1.0f);
        EXPECT_THAT(colorFromFloat.GetAsVector4(), IsClose(Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

        // Four individual floats constructor
        const Color colorFromFloats(0.5f, 0.6f, 0.7f, 1.0f);
        EXPECT_THAT(colorFromFloats.GetAsVector4(), IsClose(vectorColor));

        // Four individual uint8s constructor
        const Color colorFronUints((u8)0x7F, (u8)0x9F, (u8)0xBF, (u8)0xFF);
        EXPECT_EQ(colorFronUints.ToU32(), 0xFFBF9F7F);
    }

    TEST(MATH_Color, StaticConstruction)
    {
        // Zero color
        const Color colorZero = Color::CreateZero();
        EXPECT_THAT(colorZero.GetAsVector4(), IsClose(Vector4(0.0f, 0.0f, 0.0f, 0.0f)));

        // OneColor
        const Color colorOne = Color::CreateOne();
        EXPECT_THAT(colorOne.GetAsVector4(), IsClose(Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

        // From Float4
        const float float4Color[4] = { 0.3f, 0.4f, 0.5f, 0.8f };
        const Color colorFrom4Floats = Color::CreateFromFloat4(float4Color);
        EXPECT_THAT(colorFrom4Floats.GetAsVector4(), IsClose(Vector4(0.3f, 0.4f, 0.5f, 0.8f)));

        // From Vector3
        const Vector3 vector3Color(0.6f, 0.7f, 0.8f);
        const Color colorFromVector3 = Color::CreateFromVector3(vector3Color);
        EXPECT_THAT(colorFromVector3.GetAsVector4(), IsClose(Vector4(0.6f, 0.7f, 0.8f, 1.0f)));

        // From Vector 3 and float
        const Color colorFromVector3AndFloat = Color::CreateFromVector3AndFloat(vector3Color, 0.5f);
        EXPECT_THAT(colorFromVector3AndFloat.GetAsVector4(), IsClose(Vector4(0.6f, 0.7f, 0.8f, 0.5f)));
    }

    TEST(MATH_Color, Store)
    {
        // Store color values to float array.
        float dest[4];
        const Color colorOne = Color::CreateOne();
        colorOne.StoreToFloat4(dest);
        EXPECT_NEAR(dest[0], 1.0f, Constants::Tolerance);
        EXPECT_NEAR(dest[1], 1.0f, Constants::Tolerance);
        EXPECT_NEAR(dest[2], 1.0f, Constants::Tolerance);
        EXPECT_NEAR(dest[3], 1.0f, Constants::Tolerance);
    }

    TEST(MATH_Color, ComponentAccess)
    {
        Color testColor;

        // Float Get / Set
        testColor.SetR(0.4f);
        EXPECT_NEAR(testColor.GetR(), 0.4f, Constants::Tolerance);
        testColor.SetG(0.3f);
        EXPECT_NEAR(testColor.GetG(), 0.3f, Constants::Tolerance);
        testColor.SetB(0.2f);
        EXPECT_NEAR(testColor.GetB(), 0.2f, Constants::Tolerance);
        testColor.SetA(0.1f);
        EXPECT_NEAR(testColor.GetA(), 0.1f, Constants::Tolerance);

        // u8 Get / Set
        testColor.SetR8((u8)0x12);
        EXPECT_EQ(testColor.GetR8(), 0x12);
        testColor.SetG8((u8)0x34);
        EXPECT_EQ(testColor.GetG8(), 0x34);
        testColor.SetB8((u8)0x56);
        EXPECT_EQ(testColor.GetB8(), 0x56);
        testColor.SetA8((u8)0x78);
        EXPECT_EQ(testColor.GetA8(), 0x78);

        // Index-based element access
        testColor = Color(0.3f, 0.4f, 0.5f, 0.7f);
        EXPECT_NEAR(testColor.GetElement(0), 0.3f, Constants::Tolerance);
        EXPECT_NEAR(testColor.GetElement(1), 0.4f, Constants::Tolerance);
        EXPECT_NEAR(testColor.GetElement(2), 0.5f, Constants::Tolerance);
        EXPECT_NEAR(testColor.GetElement(3), 0.7f, Constants::Tolerance);

        testColor.SetElement(0, 0.7f);
        EXPECT_NEAR(testColor.GetR(), 0.7f, Constants::Tolerance);
        testColor.SetElement(1, 0.5f);
        EXPECT_NEAR(testColor.GetG(), 0.5f, Constants::Tolerance);
        testColor.SetElement(2, 0.4f);
        EXPECT_NEAR(testColor.GetB(), 0.4f, Constants::Tolerance);
        testColor.SetElement(3, 0.3f);
        EXPECT_NEAR(testColor.GetA(), 0.3f, Constants::Tolerance);
    }

    TEST(MATH_Color, SettersGetters)
    {
        // Vector3 getter
        const Color vectorColor = Color(0.3f, 0.4f, 0.5f, 0.7f);
        EXPECT_THAT(vectorColor.GetAsVector3(), IsClose(Vector3(0.3f, 0.4f, 0.5f)));

        // Vector 4 getter
        EXPECT_THAT(vectorColor.GetAsVector4(), IsClose(Vector4(0.3f, 0.4f, 0.5f, 0.7f)));

        // Set from single float
        Color singleValue;
        singleValue.Set(0.75f);
        EXPECT_THAT(singleValue.GetAsVector4(), IsClose(Vector4(0.75f, 0.75f, 0.75f, 0.75f)));

        // Set from 4 floats
        Color separateValues;
        separateValues.Set(0.23f, 0.45f, 0.67f, 0.89f);
        EXPECT_THAT(separateValues.GetAsVector4(), IsClose(Vector4(0.23f, 0.45f, 0.67f, 0.89f)));

        // Set from a float[4]
        float floatArray[4] = { 0.87f, 0.65f, 0.43f, 0.21f };
        Color floatArrayColor;
        floatArrayColor.Set(floatArray);
        EXPECT_TRUE(floatArrayColor.GetAsVector4(). IsClose(Vector4(0.87f, 0.65f, 0.43f, 0.21f)));

        // Set from Vector3, Alpha should be set to 1.0f
        Color vector3Color;
        vector3Color.Set(Vector3(0.2f, 0.4f, 0.6f));
        EXPECT_THAT(vector3Color.GetAsVector4(), IsClose(Vector4(0.2f, 0.4f, 0.6f, 1.0f)));

        // Set from Vector3 +_ alpha
        Color vector4Color;
        vector4Color.Set(Vector3(0.1f, 0.3f, 0.5f), 0.7f);
        EXPECT_THAT(vector4Color.GetAsVector4(), IsClose(Vector4(0.1f, 0.3f, 0.5f, 0.7f)));

        // Oddly lacking a Set() from Vector4...
    }

    TEST(MATH_Color, HueSaturationValue)
    {
        Color fromHSV(0.0f, 0.0f, 0.0f, 1.0f);

        // Check first sexant (0-60 degrees) with 0 hue, full saturation and value = red.
        fromHSV.SetFromHSVRadians(0.0f, 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 0.0f, 0.0f, 1.0f)));

        // Check the second sexant (60-120 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(72.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.8f, 1.0f, 0.0f, 1.0f)));

        // Check the third sexant (120-180 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(144.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.0f, 1.0f, 0.4f, 1.0f)));

        // Check the fourth sexant (180-240 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(216.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.0f, 0.4f, 1.0f, 1.0f)));

        // Check the fifth sexant (240-300 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(252.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.2f, 0.0f, 1.0f, 1.0f)));

        // Check the sixth sexant (300-360 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check the upper limit of the hue
        fromHSV.SetFromHSVRadians(AZ::Constants::TwoPi, 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 0.0f, 0.0f, 1.0f)));

        // Check that zero saturation causes RGB to all be value.
        fromHSV.SetFromHSVRadians(AZ::DegToRad(90.0f), 0.0f, 0.75f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.75f, 0.75f, 0.75f, 1.0f)));

        // Check that zero value causes the color to be black.
        fromHSV.SetFromHSVRadians(AZ::DegToRad(180.0f), 1.0f, 0.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.0f, 0.0f, 0.0f, 1.0f)));

        // Check a non-zero, non-one saturation
        fromHSV.SetFromHSVRadians(AZ::DegToRad(252.0f), 0.5f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.6f, 0.5f, 1.0f, 1.0f)));

        // Check a non-zero, non-one value
        fromHSV.SetFromHSVRadians(AZ::DegToRad(216.0f), 1.0f, 0.5f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.0f, 0.2f, 0.5f, 1.0f)));

        // Check a non-zero, non-one value and saturation
        fromHSV.SetFromHSVRadians(AZ::DegToRad(144.0f), 0.25f, 0.75f);
        EXPECT_THAT(fromHSV, IsClose(Color(143.44f / 255.0f, 191.25f / 255.0f, 162.56f / 255.0f, 1.0f)));

        // Check that negative hue is handled correctly (only fractional value, +1 to be positive)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(-396.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check that negative saturation is clamped to 0
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), -1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 1.0f, 1.0f, 1.0f)));

        // Check that negative value is clamped to 0
        fromHSV.SetFromHSVRadians(AZ::Constants::Pi, 1.0f, -1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.0f, 0.0f, 0.0f, 1.0f)));

        // Check that > 1 saturation is clamped to 1
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), 2.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check that > 1 value is clamped to 1
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), 1.0f, 2.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check a large hue.
        fromHSV.SetFromHSVRadians(AZ::DegToRad(3744.0f), 1.0f, 1.0f);
        EXPECT_THAT(fromHSV, IsClose(Color(0.0f, 1.0f, 0.4f, 1.0f)));
    }

    TEST(MATH_Color, EqualityComparisons)
    {
        // Equality within tolerance
        Color color1(0.1f, 0.2f, 0.3f, 0.4f);
        Color color2(0.1f, 0.2f, 0.3f, 0.4f);
        Color color3(0.12f, 0.22f, 0.32f, 0.42f);

        EXPECT_THAT(color1, IsClose(color2));
        EXPECT_FALSE(color1.IsClose(color3));
        EXPECT_THAT(color1, IsCloseTolerance(color3, 0.03f));
        EXPECT_FALSE(color1.IsClose(color3, 0.01f));

        // Zero check within tolerance
        Color zeroColor = Color::CreateZero();
        EXPECT_TRUE(zeroColor.IsZero());
        Color almostZeroColor(0.001f, 0.001f, 0.001f, 0.001f);
        EXPECT_FALSE(almostZeroColor.IsZero());
        EXPECT_TRUE(almostZeroColor.IsZero(0.01f));

        // Strict equality
        EXPECT_TRUE(color1 == color2);
        EXPECT_FALSE(color1 == color3);

        // Strict inequality
        EXPECT_FALSE(color1 != color2);
        EXPECT_TRUE(color1 != color3);
    }

    TEST(MATH_Color, LessThanComparisons)
    {
        Color color1(0.3f, 0.6f, 0.8f, 1.0f);
        Color color2(0.2f, 0.5f, 0.7f, 1.0f);
        Color color3(0.2f, 0.5f, 0.7f, 0.9f);
        Color color4(0.8f, 0.4f, 0.3f, 0.8f);

        // color 1 and two have an equal component so should not be strictly less than each other
        EXPECT_FALSE(color1.IsLessThan(color2));
        EXPECT_FALSE(color2.IsLessThan(color1));

        // color 3 should be strictly less than color 1, but not color 2
        EXPECT_TRUE(color3.IsLessThan(color1));
        EXPECT_FALSE(color3.IsLessThan(color2));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsLessThan(color1));
        EXPECT_FALSE(color4.IsLessThan(color2));
        EXPECT_FALSE(color4.IsLessThan(color3));
        EXPECT_FALSE(color1.IsLessThan(color4));
        EXPECT_FALSE(color2.IsLessThan(color4));
        EXPECT_FALSE(color3.IsLessThan(color4));

        // color 1 and two have an equal component but otherwise color 2 is less than color 1
        EXPECT_FALSE(color1.IsLessEqualThan(color2));
        EXPECT_TRUE(color2.IsLessEqualThan(color1));

        // color 3 should be less than or equal to both color 1 and color 2
        EXPECT_TRUE(color3.IsLessEqualThan(color1));
        EXPECT_TRUE(color3.IsLessEqualThan(color2));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsLessEqualThan(color1));
        EXPECT_FALSE(color4.IsLessEqualThan(color2));
        EXPECT_FALSE(color4.IsLessEqualThan(color3));
        EXPECT_FALSE(color1.IsLessEqualThan(color4));
        EXPECT_FALSE(color2.IsLessEqualThan(color4));
        EXPECT_FALSE(color3.IsLessEqualThan(color4));
    }

    TEST(MATH_Color, GreaterThanComparisons)
    {
        Color color1(0.3f, 0.6f, 0.8f, 1.0f);
        Color color2(0.2f, 0.5f, 0.7f, 1.0f);
        Color color3(0.2f, 0.5f, 0.7f, 0.9f);
        Color color4(0.8f, 0.4f, 0.3f, 0.8f);

        // color 1 and two have an equal component so should not be strictly greater than each other
        EXPECT_FALSE(color1.IsGreaterThan(color2));
        EXPECT_FALSE(color2.IsGreaterThan(color1));

        // color 1 should be strictly greater than color 3, but color 2 shouldn't
        EXPECT_TRUE(color1.IsGreaterThan(color3));
        EXPECT_FALSE(color2.IsGreaterThan(color3));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsGreaterThan(color1));
        EXPECT_FALSE(color4.IsGreaterThan(color2));
        EXPECT_FALSE(color4.IsGreaterThan(color3));
        EXPECT_FALSE(color1.IsGreaterThan(color4));
        EXPECT_FALSE(color2.IsGreaterThan(color4));
        EXPECT_FALSE(color3.IsGreaterThan(color4));

        // color 1 and two have an equal component but otherwise color 2 is less than color 1
        EXPECT_TRUE(color1.IsGreaterEqualThan(color2));
        EXPECT_FALSE(color2.IsGreaterEqualThan(color1));

        // color 1 and 2 should both be greater than or equal to color 3
        EXPECT_TRUE(color1.IsGreaterEqualThan(color3));
        EXPECT_TRUE(color2.IsGreaterEqualThan(color3));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsGreaterEqualThan(color1));
        EXPECT_FALSE(color4.IsGreaterEqualThan(color2));
        EXPECT_FALSE(color4.IsGreaterEqualThan(color3));
        EXPECT_FALSE(color1.IsGreaterEqualThan(color4));
        EXPECT_FALSE(color2.IsGreaterEqualThan(color4));
        EXPECT_FALSE(color3.IsGreaterEqualThan(color4));
    }

    TEST(MATH_Color, VectorConversions)
    {
        Vector3 vec3FromColor(Color(0.4f, 0.6f, 0.8f, 1.0f));
        EXPECT_THAT(vec3FromColor, IsClose(Vector3(0.4f, 0.6f, 0.8f)));

        Vector4 vec4FromColor(Color(0.4f, 0.6f, 0.8f, 1.0f));
        EXPECT_THAT(vec4FromColor, IsClose(Vector4(0.4f, 0.6f, 0.8f, 1.0f)));

        Color ColorfromVec3;
        ColorfromVec3 = Vector3(0.3f, 0.4f, 0.5f);
        EXPECT_THAT(ColorfromVec3.GetAsVector4(), IsClose(Vector4(0.3f, 0.4f, 0.5f, 1.0f)));
    }

    TEST(MATH_Color, LinearToGamma)
    {
        // Very dark values (these are converted differently)
        Color reallyDarkLinear(0.001234f, 0.000123f, 0.001010f, 0.5f);
        Color reallyDarkGamma = reallyDarkLinear.LinearToGamma();
        Color reallyDarkGammaCheck(0.01594328f, 0.00158916f, 0.0130492f, 0.5f);
        EXPECT_THAT(reallyDarkGamma, IsCloseTolerance(reallyDarkGammaCheck, 0.00001f));

        // Normal values
        Color normalLinear(0.123456f, 0.345678f, 0.567890f, 0.8f);
        Color normalGamma = normalLinear.LinearToGamma();
        Color normalGammaCheck(0.386281658744f, 0.622691988496f, 0.77841737783f, 0.8f);
        EXPECT_THAT(normalGamma, IsCloseTolerance(normalGammaCheck, 0.00001f));

        // Bright values
        Color brightLinear(1.234567f, 3.456789f, 5.678901f, 1.0f);
        Color brightGamma = brightLinear.LinearToGamma();
        Color brightGammaCheck(1.09681722689f, 1.71388455271f, 2.12035054203f, 1.0f);
        EXPECT_THAT(brightGamma, IsCloseTolerance(brightGammaCheck, 0.00001f));

        // Zero should stay the same
        Color zeroColor = Color::CreateZero();
        Color zeroColorGamma = zeroColor.LinearToGamma();
        EXPECT_THAT(zeroColorGamma, IsCloseTolerance(zeroColor, 0.00001f));

        // One should stay the same
        Color oneColor = Color::CreateOne();
        Color oneColorGamma = oneColor.LinearToGamma();
        EXPECT_THAT(oneColorGamma, IsCloseTolerance(oneColor, 0.00001f));
    }

    TEST(MATH_Color, GammaToLinear)
    {
        // Very dark values (these are converted differently)
        Color reallyDarkGamma(0.001234f, 0.000123f, 0.001010f, 0.5f);
        Color reallyDarkLinear = reallyDarkGamma.GammaToLinear();
        Color reallyDarkLinearCheck(0.0000955108359133f, 0.00000952012383901f, 0.000078173374613f, 0.5f);
        EXPECT_THAT(reallyDarkLinear, IsCloseTolerance(reallyDarkLinearCheck, 0.00001f));

        // Normal values
        Color normalGamma(0.123456f, 0.345678f, 0.567890f, 0.8f);
        Color normalLinear = normalGamma.GammaToLinear();
        Color normalLinearCheck(0.0140562303977f, 0.097927189487f, 0.282345816828f, 0.8f);
        EXPECT_THAT(normalLinear, IsCloseTolerance(normalLinearCheck, 0.00001f));

        // Bright values
        Color brightGamma(1.234567f, 3.456789f, 5.678901f, 1.0f);
        Color brightLinear = brightGamma.GammaToLinear();
        Color brightLinearCheck(1.61904710087f, 17.9251290437f, 58.1399365547f, 1.0f);
        EXPECT_THAT(brightLinear, IsCloseTolerance(brightLinearCheck, 0.00001f));

        // Zero should stay the same
        Color zeroColor = Color::CreateZero();
        Color zeroColorLinear = zeroColor.GammaToLinear();
        EXPECT_THAT(zeroColorLinear, IsCloseTolerance(zeroColor, 0.00001f));

        // One should stay the same
        Color oneColor = Color::CreateOne();
        Color oneColorLinear = oneColor.GammaToLinear();
        EXPECT_THAT(oneColorLinear, IsCloseTolerance(oneColor, 0.00001f));
    }

    TEST(MATH_Color, UintConversions)
    {
        // Convert to u32, floats expected to floor.
        Color colorToU32(0.23f, 0.55f, 0.88f, 1.0f);
        EXPECT_EQ(colorToU32.ToU32(), 0xFFE08C3A);

        // Convert from u32
        Color colorFromU32;
        colorFromU32.FromU32(0xFFE08C3A);
        EXPECT_THAT(colorFromU32, IsCloseTolerance(Color(0.22745098039f, 0.549019607843f, 0.87843137254f, 1.0f), 0.00001f));

        // Convert to u32 and change to gamma space at the same time.
        Color colorToU32Gamma(0.23f, 0.55f, 0.88f, 0.5f);
        EXPECT_EQ(colorToU32Gamma.ToU32LinearToGamma(), 0x7FF1C383);

        // Convert from u32 and change to linear space at the same time.
        Color colorFromU32Gamma;
        colorFromU32Gamma.FromU32GammaToLinear(0x7FF1C383);
        EXPECT_THAT(colorFromU32Gamma, IsCloseTolerance(Color(0.22696587351f, 0.54572446137f, 0.879622396888f, 0.498039215686f), 0.00001f));
    }

    TEST(MATH_Color, Lerp)
    {
        Color colorSrc(1.0f, 0.0f, 0.2f, 0.8f);
        Color colorDest(0.0f, 1.0f, 0.8f, 0.2f);

        EXPECT_THAT(colorSrc.Lerp(colorDest, 0.0f), IsClose(colorSrc));
        EXPECT_THAT(colorSrc.Lerp(colorDest, 0.5f), IsCloseTolerance(Color(0.5f, 0.5f, 0.5f, 0.5f), 0.00001f));
        EXPECT_THAT(colorSrc.Lerp(colorDest, 1.0f), IsClose(colorDest));
    }

    TEST(MATH_Color, DotProduct)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color color3(2.5f, 1.7f, 5.3f, 2.8f);

        EXPECT_NEAR(color1.Dot(color2), 0.75f, 0.0001f);
        EXPECT_NEAR(color1.Dot(color3), 4.05f, 0.0001f);
        EXPECT_NEAR(color2.Dot(color3), 6.27f, 0.0001f);

        EXPECT_NEAR(color1.Dot3(color2), 0.67f, 0.0001f);
        EXPECT_NEAR(color1.Dot3(color3), 3.77f, 0.0001f);
        EXPECT_NEAR(color2.Dot3(color3), 4.03f, 0.0001f);
    }

    TEST(MATH_Color, Addition)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color colorSum(1.1f, 1.1f, 0.6f, 0.9f);

        EXPECT_THAT(colorSum, IsCloseTolerance(color1 + color2, 0.0001f));

        color1 += color2;
        EXPECT_THAT(colorSum, IsCloseTolerance(color1, 0.0001f));
    }

    TEST(MATH_Color, Subtraction)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color colorDiff(0.1f, -0.3f, 0.0f, -0.7f);

        EXPECT_THAT(colorDiff, IsCloseTolerance(color1 - color2, 0.0001f));

        color1 -= color2;
        EXPECT_THAT(colorDiff, IsCloseTolerance(color1, 0.0001f));
    }

    TEST(MATH_Color, Multiplication)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color colorProduct(0.3f, 0.28f, 0.09f, 0.08f);
        Color color2Double(1.0f, 1.4f, 0.6f, 1.6f);

        // Product of two colors
        EXPECT_THAT(colorProduct, IsCloseTolerance(color1 * color2, 0.0001f));

        // Multiply-assignment
        color1 *= color2;
        EXPECT_THAT(colorProduct, IsCloseTolerance(color1, 0.0001f));

        // Product of color and float
        EXPECT_THAT(color2Double, IsClose(color2 * 2.0f));

        // Multiply-assignment with single float
        color2 *= 2.0f;
        EXPECT_THAT(color2Double, IsClose(color2));
    }

    TEST(MATH_Color, Division)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.8f, 0.3f, 0.8f);
        Color colorQuotient(1.2f, 0.5f, 1.0f, 0.125f);
        Color color2Half(0.25f, 0.4f, 0.15f, 0.4f);

        // Product of two colors
        EXPECT_THAT(colorQuotient, IsCloseTolerance(color1 / color2, 0.0001f));

        // Multiply-assignment
        color1 /= color2;
        EXPECT_THAT(colorQuotient, IsCloseTolerance(color1, 0.0001f));

        // Product of color and float
        EXPECT_THAT(color2Half, IsClose(color2 / 2.0f));

        // Multiply-assignment with single float
        color2 /= 2.0f;
        EXPECT_THAT(color2Half, IsClose(color2));
    }
}
