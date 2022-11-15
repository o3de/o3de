/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/PaintBrush/PaintBrush.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <Tests/PaintBrush/MockPaintBrushNotificationHandler.h>

// This set of unit tests validates that the PaintToLocation/SmoothToLocation APIs use the paint brush settings correctly.

namespace UnitTest
{
    class PaintBrushPaintSettingsTestFixture : public ScopedAllocatorSetupFixture
    {
    public:
        // Some common default values that we'll use in our tests.
        const AZ::EntityComponentIdPair EntityComponentIdPair{ AZ::EntityId(123), AZ::ComponentId(456) };
        const AZ::Vector3 TestBrushCenter{ 10.0f, 20.0f, 30.0f };
        const AZ::Vector3 TestBrushCenter2d{ 10.0f, 20.0f, 0.0f }; // This should be the same as TestBrushCenter, but with Z=0
        const AZ::Color TestColor{ 0.25f, 0.50f, 0.75f, 1.0f };

        // Useful helpers for our tests
        AzToolsFramework::PaintBrushSettings m_settings;

        // Verify that for whatever PaintBrushSettings have already been set, that both PaintToLocation() and SmoothToLocation()
        // won't produce any notifications when they're triggered, because the settings won't produce any valid points.
        void TestZeroNotificationsForPaintAndSmooth()
        {
            AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
            ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

            EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_)).Times(0);
            EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);

            paintBrush.BeginPaintMode();

            paintBrush.BeginBrushStroke(m_settings);
            paintBrush.PaintToLocation(TestBrushCenter, m_settings);
            paintBrush.EndBrushStroke();

            paintBrush.BeginBrushStroke(m_settings);
            paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
            paintBrush.EndBrushStroke();

            paintBrush.EndPaintMode();
        }

        // Validate that both PaintToLocation() and SmoothToLocation() behave the same way for the previously set PaintBrushSettings.
        // This validation only checks for valid dirtyArea and valueLookupFn results, which are common to both OnPaint() and OnSmooth()
        // notifications. The *ToLocation() call will be called once for each validationFn provided.
        using ValidationFn =
            AZStd::function<void(const AZ::Aabb& dirtyArea, AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)>;

        void ValidatePaintAndSmooth(
            AzToolsFramework::PaintBrush& paintBrush,
            ::testing::NiceMock<MockPaintBrushNotificationBusHandler>& mockHandler,
            AZStd::span<const AZ::Vector3> locations,
            AZStd::span<ValidationFn> validationFns)
        {
            AZ_Assert(locations.size() == validationFns.size(), "We should have one location for each validationFn passed in.");

            paintBrush.BeginPaintMode();

            // Verify that PaintToLocation() validates correctly.
            paintBrush.BeginBrushStroke(m_settings);
            for (size_t index = 0; index < locations.size(); index++)
            {
                EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_)).Times(1);
                ON_CALL(mockHandler, OnPaint)
                    .WillByDefault(
                        [=](const AZ::Aabb& dirtyArea,
                            AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                            [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::BlendFn& blendFn)
                        {
                            validationFns[index](dirtyArea, valueLookupFn);
                        });

                paintBrush.PaintToLocation(locations[index], m_settings);
            }
            paintBrush.EndBrushStroke();

            // Verify that SmoothToLocation() validates correctly.
            paintBrush.BeginBrushStroke(m_settings);
            for (size_t index = 0; index < locations.size(); index++)
            {
                EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);
                ON_CALL(mockHandler, OnSmooth)
                    .WillByDefault(
                        [=](const AZ::Aabb& dirtyArea,
                            AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                            [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                            [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                        {
                            validationFns[index](dirtyArea, valueLookupFn);
                        });
                paintBrush.SmoothToLocation(locations[index], m_settings);
            }
            paintBrush.EndBrushStroke();

            paintBrush.EndPaintMode();
        }

        // Test out the blendFn that we're provided from the requested blend mode by running through sets of values and blending them.
        // The caller needs to provide a verifyFn that should produce an expected value that we'll compare against.
        void TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode blendMode,
            AZStd::function<float(float baseValue, float newValue, float opacity)> verifyFn)
        {
            AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
            ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

            // Set the smooth mode to "Mean" so that we can fill the kernel values with all of the same values, which
            // lets us use the same verification function for testing the blendFn and the smoothFn since we'll have the
            // same baseValue, newValue, and opacity.
            m_settings.SetSmoothMode(AzToolsFramework::PaintBrushSmoothMode::Mean);

            m_settings.SetBlendMode(blendMode);

            paintBrush.BeginPaintMode();

            // Test the blend mode with PaintToLocation()

            EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_)).Times(1);
            ON_CALL(mockHandler, OnPaint)
                .WillByDefault(
                    [=]([[maybe_unused]] const AZ::Aabb& dirtyArea,
                        [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                        AzToolsFramework::PaintBrushNotifications::BlendFn& blendFn)
                    {
                        for (float baseValue = 0.0f; baseValue <= 1.0f; baseValue += 0.25f)
                        {
                            for (float newValue = 0.0f; newValue <= 1.0f; newValue += 0.25f)
                            {
                                for (float opacity = 0.0f; opacity < 1.0f; opacity += 0.25f)
                                {
                                    float expectedValue = AZStd::clamp(verifyFn(baseValue, newValue, opacity), 0.0f, 1.0f);
                                    EXPECT_NEAR(blendFn(baseValue, newValue, opacity), expectedValue, 0.001f);
                                }
                            }
                        }
                    });

            paintBrush.BeginBrushStroke(m_settings);
            paintBrush.PaintToLocation(TestBrushCenter, m_settings);
            paintBrush.EndBrushStroke();

            // Test the blend mode with SmoothToLocation()

            EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);
            ON_CALL(mockHandler, OnSmooth)
                .WillByDefault(
                    [=]([[maybe_unused]] const AZ::Aabb& dirtyArea,
                        [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                        [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                        AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                    {
                        for (float baseValue = 0.0f; baseValue <= 1.0f; baseValue += 0.25f)
                        {
                            for (float newValue = 0.0f; newValue <= 1.0f; newValue += 0.25f)
                            {
                                // Create a 3x3 set of kernelValues all with newValue. The mean of this will be newValue,
                                // so the output of smoothFn should be the same as blendFn for the same combinations of values.
                                AZStd::vector<float> kernelValues(9, newValue);

                                for (float opacity = 0.0f; opacity < 1.0f; opacity += 0.25f)
                                {
                                    float expectedValue = AZStd::clamp(verifyFn(baseValue, newValue, opacity), 0.0f, 1.0f);
                                    EXPECT_NEAR(smoothFn(baseValue, kernelValues, opacity), expectedValue, 0.001f);
                                }
                            }
                        }
                    });

            paintBrush.BeginBrushStroke(m_settings);
            paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
            paintBrush.EndBrushStroke();

            paintBrush.EndPaintMode();
        }
    };

    TEST_F(PaintBrushPaintSettingsTestFixture, ZeroOpacityBrushSettingCausesNoNotifications)
    {
        // If the opacity is zero (transparent), OnPaint/OnSmooth will never get called because no points can get modified.

        m_settings.SetColor(AZ::Color(0.0f));
        TestZeroNotificationsForPaintAndSmooth();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, SizeBrushSettingAffectsPaintBrush)
    {
        // The PaintBrush 'Size' setting should affect the overall size of the paint brush circle that's being used to paint/smooth.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Loop through a series of different brush radius sizes.
        for (auto& brushRadiusSize : {0.5f, 1.0f, 5.0f, 10.0f, 20.0f})
        {
            m_settings.SetSize(brushRadiusSize * 2.0f);

            ValidationFn validateFn = [=](const AZ::Aabb& dirtyArea,
                                  AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
            {
                // The dirtyArea AABB should change size based on the current brush radius size that we're using.
                EXPECT_THAT(dirtyArea, IsClose(AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, brushRadiusSize)));

                // Create a 3x3 square grid of points. Because our brush is a circle, we expect only the points along a + to be
                // returned as valid points. The corners of the square should fall outside the circle and not get returned.
                // Since we're scaling this based on the AABB, this should be checking the same relative points for each
                // brush radius.
                const AZStd::vector<AZ::Vector3> points = {
                    AZ::Vector3(dirtyArea.GetMin().GetX(), dirtyArea.GetMin().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetCenter().GetX(), dirtyArea.GetMin().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMax().GetX(), dirtyArea.GetMin().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMin().GetX(), dirtyArea.GetCenter().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetCenter().GetX(), dirtyArea.GetCenter().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMax().GetX(), dirtyArea.GetCenter().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMin().GetX(), dirtyArea.GetMax().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetCenter().GetX(), dirtyArea.GetMax().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMax().GetX(), dirtyArea.GetMax().GetY(), 0.0f),
                };
                AZStd::vector<AZ::Vector3> validPoints;
                AZStd::vector<float> opacities;
                valueLookupFn(points, validPoints, opacities);

                // We should only have the 5 points along the + in validPoints.
                const AZStd::vector<AZ::Vector3> expectedValidPoints = {
                    AZ::Vector3(dirtyArea.GetCenter().GetX(), dirtyArea.GetMin().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMin().GetX(), dirtyArea.GetCenter().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetCenter().GetX(), dirtyArea.GetCenter().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetMax().GetX(), dirtyArea.GetCenter().GetY(), 0.0f),
                    AZ::Vector3(dirtyArea.GetCenter().GetX(), dirtyArea.GetMax().GetY(), 0.0f),
                };
                EXPECT_THAT(validPoints, ::testing::Pointwise(ContainerIsClose(), expectedValidPoints));
            };

            ValidatePaintAndSmooth(paintBrush, mockHandler, { &TestBrushCenter, 1 }, { &validateFn, 1 });
        }
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, ZeroSizeBrushSettingCausesNoNotifications)
    {
        // If the brush size is zero, OnPaint/OnSmooth will never get called because no points can get modified.

        m_settings.SetSize(0.0f);
        TestZeroNotificationsForPaintAndSmooth();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, HardnessBrushSettingAffectsPaintBrush)
    {
        // The 'Hardness %' setting should apply an opacity falloff curve. It starts at the (radius * hardness%) distance from the
        // center and ends at the radius distance from the center.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        const float TestRadiusSize = 10.0f;
        m_settings.SetSize(TestRadiusSize * 2.0f);

        // Loop through a series of different hardness % values. We'll test 100% separately.
        for (auto& hardnessPercent : { 0.0f, 1.0f, 50.0f, 99.0f })
        {
            m_settings.SetHardnessPercent(hardnessPercent);

            ValidationFn validateFn =
                [=]([[maybe_unused]] const AZ::Aabb& dirtyArea, AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
            {
                // The falloff function should start at the hardness percentage from the center.
                const float falloffStart = hardnessPercent / 100.0f;

                const AZStd::vector<AZ::Vector3> points = {
                    // Test the opacity at the brush center. It should be 1.
                    TestBrushCenter2d,

                    // Test the opacity at the hardness percent (i.e. the start of the falloff). It should also be 1.
                    TestBrushCenter2d + AZ::Vector3(TestRadiusSize * falloffStart, 0.0f, 0.0f),

                    // Test the opacity halfway between the falloff and the edge.
                    // The opacity should be 0.5, because even though it's a falloff curve, the curve hits the midpoint
                    // of (0.5, 0.5).
                    TestBrushCenter2d + AZ::Vector3(TestRadiusSize * (falloffStart + ((1.0f - falloffStart) / 2.0f)), 0.0f, 0.0f),

                    // Test the opacity at the edge of the brush.
                    TestBrushCenter2d + AZ::Vector3(TestRadiusSize, 0.0f, 0.0f),
                };
                AZStd::vector<AZ::Vector3> validPoints;
                AZStd::vector<float> opacities;
                valueLookupFn(points, validPoints, opacities);

                // Only the first 3 points should be valid, since the 4th should have an opacity of 0.
                EXPECT_EQ(validPoints.size(), 3);

                // The brush should have an opacity of 1.0 from the center to the hardness % along the radius.
                // The falloff curve should hit 50% between the start of the falloff and the end.
                // The end is 0%, which won't get reported as a valid point, because it's transparent.
                const AZStd::vector<float> expectedOpacities = { 1.0f, 1.0f, 0.5f };

                EXPECT_THAT(opacities, ::testing::Pointwise(::testing::FloatNear(0.001f), expectedOpacities));
            };

            ValidatePaintAndSmooth(paintBrush, mockHandler, { &TestBrushCenter, 1 }, { &validateFn, 1 });
        }
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, FullHardnessBrushSettingHasNoFalloff)
    {
        // Verify that 100% Hardness on PaintBrushSettings means there is no falloff.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        const float TestRadiusSize = 10.0f;
        m_settings.SetSize(TestRadiusSize * 2.0f);
        m_settings.SetHardnessPercent(100.0f);

        // Verify that paint/smooth uses the hardness percent correctly.
        ValidationFn validateFn =
            [=]([[maybe_unused]] const AZ::Aabb& dirtyArea, AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
        {
            const AZStd::vector<AZ::Vector3> points = {
                // Test the opacity at the brush center + 0%, 25%, 50%, 75%, 100%
                TestBrushCenter2d,
                TestBrushCenter2d + AZ::Vector3(TestRadiusSize * 0.25f, 0.0f, 0.0f),
                TestBrushCenter2d + AZ::Vector3(TestRadiusSize * 0.50f, 0.0f, 0.0f),
                TestBrushCenter2d + AZ::Vector3(TestRadiusSize * 0.75f, 0.0f, 0.0f),
                TestBrushCenter2d + AZ::Vector3(TestRadiusSize * 1.00f, 0.0f, 0.0f),
            };
            AZStd::vector<AZ::Vector3> validPoints;
            AZStd::vector<float> opacities;
            valueLookupFn(points, validPoints, opacities);

            // All 5 points should have opacity of 1.0 when using a hardness of 100%.
            const AZStd::vector<float> expectedOpacities(5, 1.0f);

            EXPECT_THAT(opacities, ::testing::Pointwise(::testing::FloatNear(0.001f), expectedOpacities));
        };

        ValidatePaintAndSmooth(paintBrush, mockHandler, { &TestBrushCenter, 1 }, { &validateFn, 1 });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, FlowBrushSettingAffectsPaintBrush)
    {
        // The 'Flow %' setting affects the opacity of each paint circle.
        // The alpha value in the stroke color (stroke opacity) provides a constant opacity of every circle
        // in the stroke regardless of how much they overlap.
        // Flow % provides an opacity for each circle that will accumulate where they overlap. It's a non-linear accumulation,
        // because each usage of flow % will be applied to the distance between the current opacity and 1.0. For example,
        // for 10% flow starting at opacity=0:
        // opacity = 0.0 + (1 - 0.0) * 0.1 = 0.1
        // opacity = 0.1 + (1 - 0.1) * 0.1 = 0.19
        // opacity = 0.19 + (1 - 0.19) * 0.1 = 0.271
        // ...

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        const float TestRadiusSize = 10.0f;
        m_settings.SetSize(TestRadiusSize * 2.0f);

        const float TestFlowPercent = 10.0f;
        const float TestFlow = TestFlowPercent / 100.0f;
        m_settings.SetFlowPercent(TestFlowPercent);

        const float TestDistancePercent = 50.0f;
        m_settings.SetDistancePercent(TestDistancePercent);

        // The first location is an arbitrary point, and the second location
        // is one full brush circle to the right of the first one along the X axis.
        const AZ::Vector3 secondLocation = TestBrushCenter + AZ::Vector3(TestRadiusSize * 2.0f, 0.0f, 0.0f);
        AZStd::vector<AZ::Vector3> locations = { TestBrushCenter, secondLocation };

        // On the first PaintToLocation() call, we only have a single brush circle, so it should have a constant
        // opacity value that matches our flow percentage.
        ValidationFn validateFirstCallFn =
            [=]([[maybe_unused]] const AZ::Aabb& dirtyArea, AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
        {
            AZStd::vector<AZ::Vector3> points;

            // Generate a series of points that span across the center of the circle.
            for (float x = -TestRadiusSize; x <= TestRadiusSize; x += 1.0f)
            {
                points.emplace_back(TestBrushCenter2d + AZ::Vector3(x, 0.0f, 0.0f));
            }

            AZStd::vector<AZ::Vector3> validPoints;
            AZStd::vector<float> opacities;
            valueLookupFn(points, validPoints, opacities);

            // Every point we submitted should be valid.
            EXPECT_EQ(validPoints.size(), points.size());
            EXPECT_EQ(opacities.size(), points.size());

            // For the initial brush circle, every point should have the same opacity, which is our flow %.
            for (auto& opacity : opacities)
            {
                EXPECT_NEAR(opacity, TestFlow, 0.001f);
            }
        };

        /*
          On the second PaintToLocationCall(), we're going to move exactly full brush circle away along the X axis.
          However, because our distance % is set to 50%, we'll get 2 overlapping circles - 'a' and 'b' in this diagram.
          (The first circle of '.' is from the first PaintToLocation and doesn't show up in this one)
                .  .  a  a  b  b
             .     a  .  b  a     b
            .     a    .b    a     b
            .     a    .b    a     b
             .     a  .  b  a     b
                .  .  a  a  b  b

                  |-----|----|-----|
                 -2r   -r    0     r

          If the Flow % opacity is working correctly, we should end up with 10% opacity where the 'a' and 'b' circles are separate,
          and 19% opacity where the two circles overlap, because the accumulation isn't a straight addition.
          We're using 50% distance between the circles, which is equal to the brush radius.

          Since the location that we're painting to is the center of circle 'b', we expect that from that center point, along the X axis,
          (-2 * radius) to (-1 * radius) falls in circle 'a' only and should be 10%. (-1 * radius) to (0) should
          fall in both circles and be 19%. (0) to (1 * radius) falls in circle 'b' only and should be 10% again.
        */
        ValidationFn validateSecondCallFn =
            [=](const AZ::Aabb& dirtyArea, AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
        {
            AZStd::vector<AZ::Vector3> points;

            // Generate a series of points that span across the entire dirty area along the center of the circles.
            for (float x = dirtyArea.GetMin().GetX(); x <= dirtyArea.GetMax().GetX(); x += 0.25f)
            {
                points.emplace_back(AZ::Vector3(x, dirtyArea.GetCenter().GetY(), dirtyArea.GetCenter().GetZ()));
            }

            AZStd::vector<AZ::Vector3> validPoints;
            AZStd::vector<float> opacities;
            valueLookupFn(points, validPoints, opacities);

            // Every point we submitted should be valid.
            EXPECT_EQ(validPoints.size(), points.size());
            EXPECT_EQ(opacities.size(), points.size());

            for (size_t index = 0; index < validPoints.size(); index++)
            {
                float xLocation = (validPoints[index] - secondLocation).GetX();

                if (xLocation < (-TestRadiusSize))
                {
                    // Opacities in [-2*radius, -1*radius) only fall in circle 'a' and should be 10%
                    EXPECT_NEAR(opacities[index], TestFlow, 0.001f);
                }
                else if (xLocation <= 0.0f)
                {
                    // Opacities in [-1*radius, 0] fall in circle 'a' and 'b' and should be 19%
                    EXPECT_NEAR(opacities[index], TestFlow + ((1.0f - TestFlow) * TestFlow), 0.001f);
                }
                else
                {
                    // Opacities in (0, radius] only fall in circle 'b' and should be 10%
                    EXPECT_NEAR(opacities[index], TestFlow, 0.001f);
                }
            }
        };

        AZStd::vector<ValidationFn> validationFns = { validateFirstCallFn, validateSecondCallFn };

        ValidatePaintAndSmooth(paintBrush, mockHandler, locations, validationFns);
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, ZeroFlowBrushSettingCausesNoNotifications)
    {
        // If the flow % is zero, OnPaint/OnSmooth will never get called because no points can get modified.

        m_settings.SetFlowPercent(0.0f);
        TestZeroNotificationsForPaintAndSmooth();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, DistanceBrushSettingAffectsPaintBrush)
    {
        // The 'Distance %' setting affects how far apart each paint circle is applied during a brush movement.
        // The % is in terms of the brush size, so 50% produces circles that overlap by 50%, 100% produces circles that
        // perfectly don't overlap, 200% produces circles with exactly one empty circle between each one, etc.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        const float TestRadiusSize = 10.0f;
        m_settings.SetSize(TestRadiusSize * 2.0f);

        // Choose a second location that's sufficiently far away that we'll get multiple brush circles for each of our
        // chosen distance % values.
        AZStd::vector<AZ::Vector3> locations = { TestBrushCenter, TestBrushCenter + AZ::Vector3(TestRadiusSize * 10.0f, 0.0f, 0.0f) };

        for (auto& distancePercent : {1.0f, 10.0f, 50.0f, 100.0f, 300.0f})
        {
            m_settings.SetDistancePercent(distancePercent);

            // On the first *ToLocation() call, we only have a single brush circle, so it should have a constant
            // opacity value that matches our flow percentage.
            ValidationFn validateFirstCallFn =
                [=](const AZ::Aabb& dirtyArea, [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
            {
                // On the first call, the dirtyArea AABB should match the size of the brush.
                EXPECT_THAT(dirtyArea, IsClose(AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, TestRadiusSize)));
            };

            ValidationFn validateSecondCallFn =
                [=](const AZ::Aabb& dirtyArea, [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn)
            {
                // On the second call, a number of brush circles will be applied based on the distance %. The first
                // brush circle in this call will start distance % further than the left edge of our initial circle.
                const float initialStartX = (TestBrushCenter2d.GetX() - TestRadiusSize);
                const float expectedStartX = initialStartX + (TestRadiusSize * 2.0f) * (distancePercent / 100.0f);

                EXPECT_NEAR(dirtyArea.GetMin().GetX(), expectedStartX, 0.001f);
            };

            AZStd::vector<ValidationFn> validationFns = { validateFirstCallFn, validateSecondCallFn };

            ValidatePaintAndSmooth(paintBrush, mockHandler, locations, validationFns);
        }
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, ZeroDistanceBrushSettingCausesNoNotifications)
    {
        // If the distance % is zero, OnPaint/OnSmooth will never get called because no points can get modified.

        m_settings.SetDistancePercent(0.0f);
        TestZeroNotificationsForPaintAndSmooth();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, NormalBlendBrushSettingIsCorrect)
    {
        // The 'Normal' Blend brush setting is just a standard lerp.
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Normal,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, newValue, opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, AddBlendBrushSettingIsCorrect)
    {
        // The 'Add' Blend brush setting lerps between the base and 'base + new'.
        // Note that we specifically do NOT expect it to clamp the add. This matches Photoshop's behavior,
        // but other paint programs vary in their choice here.
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Add,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, baseValue + newValue, opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, SubtractBlendBrushSettingIsCorrect)
    {
        // The 'Subtract' Blend brush setting lerps between the base and 'base - new'
        // Note that we specifically do NOT expect it to clamp the subtract. This matches Photoshop's behavior,
        // but other paint programs vary in their choice here.
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Subtract,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, baseValue - newValue, opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, MultiplyBlendBrushSettingIsCorrect)
    {
        // The 'Multiply' Blend brush setting lerps between the base and 'base * new'
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Multiply,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, baseValue * newValue, opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, ScreenBlendBrushSettingIsCorrect)
    {
        // The 'Screen' Blend brush setting lerps between the base and '1 - (1 - base) * (1 - new)'
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Screen,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, 1.0f - ((1.0f - baseValue) * (1.0f - newValue)), opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, DarkenBlendBrushSettingIsCorrect)
    {
        // The 'Darken' Blend brush setting lerps between the base and 'min(base, new)'
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Darken,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, AZStd::min(baseValue, newValue), opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, LightenBlendBrushSettingIsCorrect)
    {
        // The 'Lighten' Blend brush setting lerps between the base and 'max(base, new)'
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Lighten,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, AZStd::max(baseValue, newValue), opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, AverageBlendBrushSettingIsCorrect)
    {
        // The 'Average' Blend brush setting lerps between the base and '(base + new) / 2'
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Average,
            [](float baseValue, float newValue, float opacity) -> float
            {
                return AZStd::lerp(baseValue, (baseValue + newValue) / 2.0f, opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, OverlayBlendBrushSettingIsCorrect)
    {
        // The 'Overlay' Blend brush setting lerps between the base and the following:
        // if base >= 0.5 : (1 - (2 * (1 - base) * (1 - new)))
        // if base <  0.5 : 2 * base * new
        TestBlendModeForPaintAndSmooth(
            AzToolsFramework::PaintBrushBlendMode::Overlay,
            [](float baseValue, float newValue, float opacity) -> float
            {
                if (baseValue >= 0.5f)
                {
                    return AZStd::lerp(baseValue, (1.0f - (2.0f * (1.0f - baseValue) * (1.0f - newValue))), opacity);
                }

                return AZStd::lerp(baseValue, 2.0f * baseValue * newValue, opacity);
            });
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, SmoothingRadiusSettingAffectsSmoothBrush)
    {
        // The 'Smoothing Radius' setting affects how many values are blended together to produce a smoothed result value.
        // The values should be an NxN square, where N = (radius * 2) + 1.
        // Radius 1 = 3x3 square. Radius 2 = 5x5 square. Radius 3 = 7x7 square. etc.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Set the smoothing mode to "Mean" so that we have an easily-predictable result.
        m_settings.SetSmoothMode(AzToolsFramework::PaintBrushSmoothMode::Mean);

        paintBrush.BeginPaintMode();

        for (int32_t radius = 1; radius <= 5; radius++)
        {
            m_settings.SetSmoothingRadius(radius);

            EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);
            ON_CALL(mockHandler, OnSmooth)
                .WillByDefault(
                    [=]([[maybe_unused]] const AZ::Aabb& dirtyArea,
                        [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                        AZStd::span<const AZ::Vector3> valuePointOffsets,
                        AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                    {
                        const size_t kernelSize1d = (radius * 2) + 1;
                        const size_t expectedKernelSize = kernelSize1d * kernelSize1d;

                        // We expect the number of point offsets to match the NxN square size caused by our radius setting.
                        EXPECT_EQ(valuePointOffsets.size(), expectedKernelSize);

                        // Verify that the actual offsets we've been given go from -radius to radius in each direction.
                        size_t index = 0;
                        for (float y = -radius; y <= radius; y++)
                        {
                            for (float x = -radius; x <= radius; x++)
                            {
                                EXPECT_THAT(valuePointOffsets[index], IsClose(AZ::Vector3(x, y, 0.0f)));
                                index++;
                            }
                        }

                        // Create a set of kernelValues that's NxN in size and all 0's except that last value, which is 1.
                        // Since our smoothing mode is "Mean", we should get a smoothed value of 1 / (NxN) if all of the kernelValues
                        // are used in smoothing.
                        AZStd::vector<float> kernelValues(valuePointOffsets.size(), 0.0f);
                        kernelValues.back() = 1.0f;

                        const float expectedResult = 1.0f / valuePointOffsets.size();

                        float smoothedValue = smoothFn(0.0f, kernelValues, 1.0f);
                        EXPECT_NEAR(smoothedValue, expectedResult, 0.001f);
                    });

            paintBrush.BeginBrushStroke(m_settings);
            paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
            paintBrush.EndBrushStroke();
        }

        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, GaussianSmoothModeIsCorrect)
    {
        // Verify that the Gaussian Smoothing mode produces the expected results.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Use Gaussian with a 3x3 matrix for easily-testable results.
        m_settings.SetSmoothMode(AzToolsFramework::PaintBrushSmoothMode::Gaussian);
        m_settings.SetSmoothingRadius(1);

        paintBrush.BeginPaintMode();

        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);
        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // It's a bit tricky to validate Gaussian smoothing without just recreating the Gaussian calculations,
                    // so we'll use "golden values" that are the precomputed 3x3 Gaussian matrix with known-good values.
                    AZStd::vector<float> expectedGaussianMatrix = {
                        0.0751136f, 0.1238414f, 0.0751136f,
                        0.1238414f, 0.2041799f, 0.1238414f,
                        0.0751136f, 0.1238414f, 0.0751136f,
                    };

                    // Loop through and try smoothing with all values set to 0 except for one.
                    // The result should match each value in our Gaussian matrix.
                    for (size_t index = 0; index < 9; index++)
                    {
                        AZStd::vector<float> kernelValues(9, 0.0f);
                        kernelValues[index] = 1.0f;

                        const float smoothedValue = smoothFn(0.0f, kernelValues, 1.0f);
                        EXPECT_NEAR(smoothedValue, expectedGaussianMatrix[index], 0.001f);
                    }
                });

        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
        paintBrush.EndBrushStroke();

        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, MeanSmoothModeIsCorrect)
    {
        // Verify that the Mean Smoothing mode produces the expected results.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Use Mean with a 3x3 matrix for easily-testable results.
        m_settings.SetSmoothMode(AzToolsFramework::PaintBrushSmoothMode::Mean);
        m_settings.SetSmoothingRadius(1);

        paintBrush.BeginPaintMode();

        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);
        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // Loop through and try smoothing with all values set to 0 except for one.
                    // The result should always be 1/9, since we're averaging all 9 values.
                    for (size_t index = 0; index < 9; index++)
                    {
                        const float expectedResult = 1.0f / 9.0f;

                        AZStd::vector<float> kernelValues(9, 0.0f);
                        kernelValues[index] = 1.0f;

                        const float smoothedValue = smoothFn(0.0f, kernelValues, 1.0f);
                        EXPECT_NEAR(smoothedValue, expectedResult, 0.001f);
                    }
                });

        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
        paintBrush.EndBrushStroke();

        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintSettingsTestFixture, MedianSmoothModeIsCorrect)
    {
        // Verify that the Median Smoothing mode produces the expected results.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Use Median with a 3x3 matrix for easily-testable results.
        m_settings.SetSmoothMode(AzToolsFramework::PaintBrushSmoothMode::Median);
        m_settings.SetSmoothingRadius(1);

        paintBrush.BeginPaintMode();

        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);
        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // Set our kernel values to 0.0, 0.01, 0.02, 0.03, 0.04, 0.5, 0.6, 0.7, 0.8 in scrambled order.
                    // The middle value should be 0.04. These values are non-linear to ensure that we're
                    // not taking the average of the values, and 0.04 is not the center value to ensure
                    // that we're still finding it correctly.
                    AZStd::vector<float> kernelValues = { 0.03f, 0.04f, 0.8f, 0.01f, 0.6f, 0.5f, 0.7f, 0.0f, 0.02f };
                    const float expectedResult = 0.04f;

                    const float smoothedValue = smoothFn(0.0f, kernelValues, 1.0f);
                    EXPECT_NEAR(smoothedValue, expectedResult, 0.001f);
                });

        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
        paintBrush.EndBrushStroke();

        paintBrush.EndPaintMode();
    }
} // namespace UnitTest
