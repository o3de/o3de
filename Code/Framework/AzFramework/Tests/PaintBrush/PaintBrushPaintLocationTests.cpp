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

// This set of unit tests validates that the PaintToLocation API trivially works and calculates location changes correctly.

namespace UnitTest
{
    class PaintBrushPaintLocationTestFixture : public LeakDetectionFixture
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

    TEST_F(PaintBrushPaintLocationTestFixture, PaintToLocationAtSingleLocationFunctionsCorrectly)
    {
        // This tests all of the basic PaintToLocation() functionality:
        // - It will call OnPaint with the correct dirty area for the brush settings and initial location
        // - The valueLookupFn will only return valid points that occur within the brush.
        // - The blendFn will blend two values together.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        const float TestBrushRadius = 1.0f;
        m_settings.SetSize(TestBrushRadius * 2.0f);

        EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);

        ON_CALL(mockHandler, OnPaint)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Color& color,
                    const AZ::Aabb& dirtyArea,
                    AzFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    AzFramework::PaintBrushNotifications::BlendFn& blendFn)
                {
                    // The OnPaint method for a listener to the PaintBrushNotificationBus should work as follows:
                    // - It should receive a dirtyArea AABB that contains the region that's been painted.
                    // - For each point that the listener cares about in that region, it should call valueLookupFn() to find
                    //   out which points actually fall within the paintbrush, and what the opacities of those points are.
                    // - For each valid point and opacity, the listener should call blendFn() to blend the values together.

                    // Validate the dirtyArea AABB:
                    // We expect the AABB to be centered around the TestBrushCenter but with a Z value of 0
                    // because we only support painting in 2D for now. The radius of the AABB should match the radius of our paintbrush.
                    EXPECT_THAT(dirtyArea, IsClose(AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, TestBrushRadius)));

                    // Validate the valueLookupFn:
                    // Create a 3x3 square grid of points. Because our brush is a circle, we expect only the points in along a + to be
                    // returned as valid points. The corners of the square should fall outside the circle and not get returned.
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

                    // We should only have 5 opacities, and they should all be 1.0 because we haven't adjusted any brush settings.
                    EXPECT_EQ(opacities.size(), 5);
                    for (auto& opacity : opacities)
                    {
                        EXPECT_NEAR(opacity, 1.0f, 0.001f);
                    }

                    // Validate the blendFn:
                    // By default, it should be a normal blend, so verify that a base value of 0, a new value of 1, and an opacity of 0.5
                    // gives us back a blended value of 0.5.
                    const float blend = blendFn(0.0f, 1.0f, 0.5f);
                    EXPECT_NEAR(blend, 0.5f, 0.001f);
                });


        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.PaintToLocation(TestBrushCenter, m_settings);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintLocationTestFixture, PaintToLocationWithSmallMovementDoesNotTriggerPainting)
    {
        // This verifies that if the distance between two PaintToLocation calls is small enough, it won't trigger
        // an OnPaint. "small" is defined as less than (brush size * distance %).

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Set the distance between brush stamps to 50%
        const float TestDistancePercent = 50.0f;
        m_settings.SetDistancePercent(TestDistancePercent);

        // Set the brush radius to 1 meter (diameter is 2 meters)
        const float TestBrushRadius = 1.0f;
        m_settings.SetSize(TestBrushRadius * 2.0f);

        // The distance we expect to need to move to trigger another paint call is 50% of our brush size.
        const float DistanceToTriggerSecondCall = TestBrushRadius * 2.0f * (TestDistancePercent / 100.0f);

        // Choose a second brush center location that's just slightly under the threshold that should be needed to trigger a second
        // OnPaint call.
        const AZ::Vector3 tooSmallSecondLocation = TestBrushCenter + AZ::Vector3(DistanceToTriggerSecondCall - 0.01f, 0.0f, 0.0f);

        // We expect to get called only once for our initial PaintToLocation(); the second PaintToLocation() won't
        // have moved far enough to trigger a second OnPaint call.
        EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.PaintToLocation(TestBrushCenter, m_settings);
        paintBrush.PaintToLocation(tooSmallSecondLocation, m_settings);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();

        // Try the test again, this time moving exactly the amount we need to so that we trigger a second call.
        // (We do this to verify that we've correctly identified the threshold under which we should not trigger another OnPaint)
        const AZ::Vector3 largeEnoughSecondLocation = TestBrushCenter + AZ::Vector3(DistanceToTriggerSecondCall, 0.0f, 0.0f);
        EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.PaintToLocation(TestBrushCenter, m_settings);
        paintBrush.PaintToLocation(largeEnoughSecondLocation, m_settings);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintLocationTestFixture, PaintToLocationSecondMovementDoesNotIncludeFirstCircle)
    {
        // When painting, the first PaintToLocation call should just contain a single paint stamp at the passed-in location.
        // The second PaintToLocation call should contain paint stamps from the first location to the second, but should NOT
        // have a second paint stamp at the first location. Ex:
        //     O            <- first PaintToLocation
        //     -OOO         <- second PaintToLocation
        // If the Distance % is anything less than 100% in the paint brush settings, the Os will overlap.
        // We'll set it to 100% just to make it obvious that we've gotten the correct result.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Set the distance between brush stamps to 100%
        const float TestDistancePercent = 100.0f;
        m_settings.SetDistancePercent(TestDistancePercent);

        // Set the brush radius to 1 meter (diameter is 2 meters)
        const float TestBrushRadius = 1.0f;
        const float TestBrushSize = TestBrushRadius * 2.0f;
        m_settings.SetSize(TestBrushSize);

        // Choose a second brush center location that's 3 full brush stamps away. This should give us a total
        // of 4 brush stamps that get painted between the two calls.
        const AZ::Vector3 secondLocation = TestBrushCenter + AZ::Vector3(TestBrushSize * 3.0f, 0.0f, 0.0f);

        // We expect to get two OnPaint calls.
        EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        ON_CALL(mockHandler, OnPaint)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Color& color, const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::BlendFn& blendFn)
                {
                    // On the first PaintToLocation, we expect to get a dirtyArea that exactly fits our first paint brush stamp.
                    const AZ::Aabb expectedFirstDirtyArea = AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, TestBrushRadius);
                    EXPECT_THAT(dirtyArea, IsClose(expectedFirstDirtyArea));
                });

        paintBrush.PaintToLocation(TestBrushCenter, m_settings);

        ON_CALL(mockHandler, OnPaint)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Color& color, const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::BlendFn& blendFn)
                {
                    // On the second PaintToLocation, we expect the dirtyArea to only contain the next 3 paint brush stamps,
                    // but not the first one.
                    const AZ::Vector3 stampIncrement = AZ::Vector3(TestBrushSize, 0.0f, 0.0f);
                    AZ::Aabb expectedSecondDirtyArea = AZ::Aabb::CreateCenterRadius(TestBrushCenter2d + stampIncrement, TestBrushRadius);
                    expectedSecondDirtyArea.AddAabb(
                        AZ::Aabb::CreateCenterRadius(TestBrushCenter2d + (3.0f * stampIncrement), TestBrushRadius));
                    EXPECT_THAT(dirtyArea, IsClose(expectedSecondDirtyArea));
                });

        paintBrush.PaintToLocation(secondLocation, m_settings);

        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintLocationTestFixture, EyedropperDoesNotAffectPaintToLocation)
    {
        // When painting, we should be able to call UseEyedropper at any arbitrary location without affecting the current
        // state of PaintToLocation.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Set the brush radius to 1 meter (diameter is 2 meters)
        const float TestBrushRadius = 1.0f;
        const float TestBrushSize = TestBrushRadius * 2.0f;
        m_settings.SetSize(TestBrushSize);

        // Choose a second brush center location that's 2 full brush stamps away in the X direction only.
        AZ::Vector3 secondLocation = TestBrushCenter + AZ::Vector3(TestBrushSize * 2.0f, 0.0f, 0.0f);

        // We expect to get two OnPaint calls.
        EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        ON_CALL(mockHandler, OnPaint)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Color& color, const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::BlendFn& blendFn)
                {
                    // We expect that the Y value for our dirtyArea won't be changed even though we'll call UseEyedropper
                    // with a large Y value in-between the two paint calls.
                    EXPECT_NEAR(dirtyArea.GetMin().GetY(), TestBrushCenter.GetY() - TestBrushRadius, 0.01f);
                    EXPECT_NEAR(dirtyArea.GetMax().GetY(), TestBrushCenter.GetY() + TestBrushRadius, 0.01f);
                });

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        paintBrush.PaintToLocation(TestBrushCenter, m_settings);

        // Call UseEyeDropper with a large Y value so that we can easily detect if it affected our PaintToLocation calls.
        [[maybe_unused]] AZ::Color color = paintBrush.UseEyedropper(TestBrushCenter + AZ::Vector3(0.0f, 1000.0f, 0.0f));

        paintBrush.PaintToLocation(secondLocation, m_settings);

        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushPaintLocationTestFixture, ResetBrushStrokeTrackingWorksCorrectly)
    {
        // If ResetBrushStrokeTracking is called in-between two calls to PaintToLocation within a brush stroke,
        // there should be a discontinuity between the two locations as if the brush has been picked up and put back down.
        // i.e. Instead of 'OOOOOO' between two locations it should create 'O     O'.
        // This is typically used for handling things like leaving the edge of the image at one location and coming back onto
        // the image at a different location.

        AzFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Set the distance between brush stamps to 100%
        const float TestDistancePercent = 100.0f;
        m_settings.SetDistancePercent(TestDistancePercent);

        // Set the brush radius to 1 meter (diameter is 2 meters)
        const float TestBrushRadius = 1.0f;
        const float TestBrushSize = TestBrushRadius * 2.0f;
        m_settings.SetSize(TestBrushSize);

        // Choose a second brush center location that's 10 full brush stamps away to make it obvious whether or not
        // there are any points tracked between the two locations.
        const AZ::Vector3 secondLocation = TestBrushCenter + AZ::Vector3(TestBrushSize * 10.0f, 0.0f, 0.0f);

        // We expect to get two OnPaint calls.
        EXPECT_CALL(mockHandler, OnPaint(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        ON_CALL(mockHandler, OnPaint)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Color& color, const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::BlendFn& blendFn)
                {
                    // On the first PaintToLocation, we expect to get a dirtyArea that exactly fits our first paint brush stamp.
                    const AZ::Aabb expectedFirstDirtyArea = AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, TestBrushRadius);
                    EXPECT_THAT(dirtyArea, IsClose(expectedFirstDirtyArea));
                });

        paintBrush.PaintToLocation(TestBrushCenter, m_settings);

        // Reset the brush stroke tracking, so that the next location will look like the start of a stroke again.
        paintBrush.ResetBrushStrokeTracking();

        ON_CALL(mockHandler, OnPaint)
            .WillByDefault(
                [=]([[maybe_unused]] const AZ::Color& color, const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AzFramework::PaintBrushNotifications::BlendFn& blendFn)
                {
                    // On the second PaintToLocation, we expect to get a dirtyArea that exactly fits our second paint brush stamp.
                    // It should *not* include any of the space bewteen the first and the second brush stamp.
                    const AZ::Vector3 secondLocation2d(AZ::Vector2(secondLocation), 0.0f);
                    const AZ::Aabb expectedSecondDirtyArea = AZ::Aabb::CreateCenterRadius(secondLocation2d, TestBrushRadius);
                    EXPECT_THAT(dirtyArea, IsClose(expectedSecondDirtyArea));
                });

        paintBrush.PaintToLocation(secondLocation, m_settings);

        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }
} // namespace UnitTest
