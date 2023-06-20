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
#include <AzFramework/PaintBrush/PaintBrush.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <Tests/PaintBrush/MockPaintBrushNotificationHandler.h>

/*
This set of unit tests validates that the basic PaintBrush functionality and notifications work:
- Begin/End Paint Mode
- Begin/End Brush Stroke
- Use Eyedropper
*/

namespace UnitTest
{
    class PaintBrushTestFixture : public LeakDetectionFixture
    {
    public:

        // Some common default values that we'll use in our tests.
        const AZ::EntityComponentIdPair EntityComponentIdPair{ AZ::EntityId(123), AZ::ComponentId(456) };
        const AZ::Vector3 TestBrushCenter{ 10.0f, 20.0f, 30.0f };
        const AZ::Vector3 TestBrushCenter2d{ 10.0f, 20.0f, 0.0f };  // This should be the same as TestBrushCenter, but with Z=0
        const AZ::Color TestColor{ 0.25f, 0.50f, 0.75f, 1.0f };

        // Useful helpers for our tests
        AzFramework::PaintBrushSettings m_settings;
    };

    TEST_F(PaintBrushTestFixture, TrivialConstruction)
    {
        // We can construct / destroy a PaintBrush without any errors.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
    }

    TEST_F(PaintBrushTestFixture, BeginAndEndPaintModeMustBePairedCorrectly)
    {
        // BeginPaintMode() / EndPaintMode() can only be called in the correct order in pairs.
        // They will error if they're called in the wrong order or more than once in a row.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);

        // End before Begin should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.EndPaintMode();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        paintBrush.BeginPaintMode();

        // Calling Begin twice should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.BeginPaintMode();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        paintBrush.EndPaintMode();

        // Calling End twice should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.EndPaintMode();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(PaintBrushTestFixture, IsInPaintModeFunctionsCorrectly)
    {
        // IsInPaintMode() correctly identifies when we're between a BeginPaintMode() and EndPaintMode() call.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);

        EXPECT_FALSE(paintBrush.IsInPaintMode());
        paintBrush.BeginPaintMode();
        EXPECT_TRUE(paintBrush.IsInPaintMode());
        paintBrush.EndPaintMode();
        EXPECT_FALSE(paintBrush.IsInPaintMode());
    }

    TEST_F(PaintBrushTestFixture, BeginAndEndBrushStrokeMustBePairedCorrectlyInsidePaintMode)
    {
        // BeginBrushStroke() / EndBrushStroke() can only be called in the correct order in pairs.
        // They will error if they're called in the wrong order, more than once in a row, or outside of BeginPaintMode().

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);

        // BeginBrushStroke without BeginPaintMode should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.BeginBrushStroke(m_settings);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        paintBrush.BeginPaintMode();

        // End before Begin should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.EndBrushStroke();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        paintBrush.BeginBrushStroke(m_settings);

        // Calling Begin twice should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.BeginBrushStroke(m_settings);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        paintBrush.EndBrushStroke();

        // Calling End twice should assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.EndBrushStroke();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushTestFixture, IsInBrushStrokeFunctionsCorrectly)
    {
        // IsInBrushStroke() correctly identifies when we're between a BeginBrushStroke() and EndBrushStroke() call.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);

        paintBrush.BeginPaintMode();

        EXPECT_FALSE(paintBrush.IsInBrushStroke());
        paintBrush.BeginBrushStroke(m_settings);
        EXPECT_TRUE(paintBrush.IsInBrushStroke());
        paintBrush.EndBrushStroke();
        EXPECT_FALSE(paintBrush.IsInBrushStroke());

        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushTestFixture, PaintModeNotifiesCorrectly)
    {
        // BeginPaintMode() / EndPaintMode() should call the PaintBrushNotificationBus with OnPaintModeBegin() / OnPaintModeEnd().

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        EXPECT_CALL(mockHandler, OnPaintModeBegin()).Times(1);
        paintBrush.BeginPaintMode();

        EXPECT_CALL(mockHandler, OnPaintModeEnd()).Times(1);
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushTestFixture, BrushStrokeNotifiesCorrectly)
    {
        // BeginBrushStroke() / EndBrushStroke() should call the PaintBrushNotificationBus with OnBrushStrokeBegin() / OnBrushStrokeEnd().
        // OnBrushStrokeBegin should be passed the current color in the PaintBrushSettings that we called BeginBrushStroke() with.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        m_settings.SetColor(TestColor);

        EXPECT_CALL(mockHandler, OnBrushStrokeBegin(::testing::_)).Times(1);

        ON_CALL(mockHandler, OnBrushStrokeBegin)
            .WillByDefault(
                [=](const AZ::Color& strokeColor)
                {
                    EXPECT_THAT(TestColor, IsClose(strokeColor));
                    return TestColor;
                });

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        EXPECT_CALL(mockHandler, OnBrushStrokeEnd()).Times(1);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushTestFixture, UseEyedropperFunctionsCorrectlyOutsideOfPaintMode)
    {
        // UseEyedropper should return the expected color for the given world position even if we're not currently
        // in Paint Mode or inside a Brush Stroke.
        // This verifies that PaintBrushNotificationBus::OnGetColor is called with the expected world position and that the color
        // returned from UseEyedropper matches the one we returned from OnGetColor.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        ON_CALL(mockHandler, OnGetColor)
            .WillByDefault(
                [=](const AZ::Vector3& brushCenterPoint)
                {
                    // We expect the brush center to match what we passed in but with a Z value of 0
                    // because we only support painting in 2D for now.
                    EXPECT_THAT(brushCenterPoint, IsClose(TestBrushCenter2d));
                    return TestColor;
                });


        // Note that UseEyedropper() can be called without being in a paint mode or brush stroke.
        const AZ::Color color = paintBrush.UseEyedropper(TestBrushCenter);

        EXPECT_THAT(color, IsClose(TestColor));
    }

    TEST_F(PaintBrushTestFixture, UseEyedropperFunctionsCorrectlyInsideOfPaintMode)
    {
        // UseEyedropper should return the expected color for the given world position even if we are currently
        // in Paint Mode and inside a Brush Stroke.
        // This verifies that PaintBrushNotificationBus::OnGetColor is called with the expected world position and that the color
        // returned from UseEyedropper matches the one we returned from OnGetColor.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        ON_CALL(mockHandler, OnGetColor)
            .WillByDefault(
                [=](const AZ::Vector3& brushCenterPoint)
                {
                    // We expect the brush center to match what we passed in but with a Z value of 0
                    // because we only support painting in 2D for now.
                    EXPECT_THAT(brushCenterPoint, IsClose(TestBrushCenter2d));
                    return TestColor;
                });

        // UseEyedropper should still work even if a paint mode and brush stroke is active.
        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        const AZ::Color color = paintBrush.UseEyedropper(TestBrushCenter);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();

        EXPECT_THAT(color, IsClose(TestColor));
    }

    TEST_F(PaintBrushTestFixture, PaintToLocationMustBeUsedWithinBrushStroke)
    {
        // PaintToLocation() can only be called if we're currently within a brush stroke.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        AZ_TEST_START_TRACE_SUPPRESSION;
        paintBrush.PaintToLocation(TestBrushCenter, m_settings);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }
} // namespace UnitTest
