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

// This set of unit tests validates that the SmoothToLocation API trivially works and calculates location changes correctly.

namespace UnitTest
{
    class PaintBrushSmoothLocationTestFixture : public ScopedAllocatorSetupFixture
    {
    public:

        // Some common default values that we'll use in our tests.
        const AZ::EntityComponentIdPair EntityComponentIdPair{ AZ::EntityId(123), AZ::ComponentId(456) };
        const AZ::Vector3 TestBrushCenter{ 10.0f, 20.0f, 30.0f };
        const AZ::Vector3 TestBrushCenter2d{ 10.0f, 20.0f, 0.0f };  // This should be the same as TestBrushCenter, but with Z=0
        const AZ::Color TestColor{ 0.25f, 0.50f, 0.75f, 1.0f };

        // Useful helpers for our tests
        AzToolsFramework::PaintBrushSettings m_settings;
    };

    TEST_F(PaintBrushSmoothLocationTestFixture, SmoothToLocationAtSingleLocationFunctionsCorrectly)
    {
        // This tests all of the basic SmoothToLocation() functionality:
        // - It will call OnSmooth with the correct dirty area for the brush settings and initial location
        // - The valueLookupFn will only return valid points that occur within the brush.
        // - The smoothFn will smooth values together.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        const float TestBrushRadius = 1.0f;
        m_settings.SetSize(TestBrushRadius * 2.0f);

        // We'll set the smooth mode to "Mean" just so that it's easy to verify that the smoothFn trivially works.
        m_settings.SetSmoothMode(AzToolsFramework::PaintBrushSmoothMode::Mean);

        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);

        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=](const AZ::Aabb& dirtyArea,
                    AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    AZStd::span<const AZ::Vector3> valuePointOffsets,
                    AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // The OnSmooth method for a listener to the PaintBrushNotificationBus should work as follows:
                    // - It should receive a dirtyArea AABB that contains the region that's been smoothed.
                    // - For each point that the listener cares about in that region, it should call valueLookupFn() to find
                    //   out which points actually fall within the paintbrush, and what the opacities of those points are.
                    // - For each valid point and opacity, the listener should gather all of the points around the point based
                    //   on the relative valuePointOffsets, and then call smoothFn with all of those points to get a smoothed value.

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

                    // By default, the smoothing brush uses a 3x3 kernel, so we expect our relative offsets to be -1 to 1
                    // in each direction.
                    const AZStd::vector<AZ::Vector3> expectedPointOffsets = {
                        AZ::Vector3(-1.0f, -1.0f, 0.0f), AZ::Vector3(0.0f, -1.0f, 0.0f), AZ::Vector3(1.0f, -1.0f, 0.0f),
                        AZ::Vector3(-1.0f, 0.0f, 0.0f),  AZ::Vector3(0.0f, 0.0f, 0.0f),  AZ::Vector3(1.0f, 0.0f, 0.0f),
                        AZ::Vector3(-1.0f, 1.0f, 0.0f),  AZ::Vector3(0.0f, 1.0f, 0.0f),  AZ::Vector3(1.0f, 1.0f, 0.0f),
                    };
                    EXPECT_THAT(valuePointOffsets, ::testing::Pointwise(ContainerIsClose(), expectedPointOffsets));

                    const float baseValue = 1.0f;

                    // We'll set our kernel to only have a single value of 1. The mean should be 1/9.
                    float kernelValues[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
                    const float expectedMean = 1.0f / 9.0f;

                    // With full opacity, we should just get back the mean of our kernel values.
                    const float smoothedValue = smoothFn(baseValue, kernelValues, 1.0f);
                    EXPECT_NEAR(smoothedValue, expectedMean, 0.01f);

                    // With half opacity, we should get back a value halfway between the mean and 1.0f.
                    const float partialSmoothedValue = smoothFn(baseValue, kernelValues, 0.5f);
                    EXPECT_NEAR(partialSmoothedValue, expectedMean + ((1.0f - expectedMean) / 2.0f), 0.01f);
                });


        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushSmoothLocationTestFixture, SmoothToLocationWithSmallMovementDoesNotTriggerPainting)
    {
        // This verifies that if the distance between two SmoothToLocation calls is small enough, it won't trigger
        // an OnSmooth. "small" is defined as less than (brush size * distance %).

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
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

        // We expect to get called only once for our initial SmoothToLocation(); the second SmoothToLocation() won't
        // have moved far enough to trigger a second OnSmooth call.
        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
        paintBrush.SmoothToLocation(tooSmallSecondLocation, m_settings);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();

        // Try the test again, this time moving exactly the amount we need to so that we trigger a second call.
        // (We do this to verify that we've correctly identified the threshold under which we should not trigger another OnSmooth)
        const AZ::Vector3 largeEnoughSecondLocation = TestBrushCenter + AZ::Vector3(DistanceToTriggerSecondCall, 0.0f, 0.0f);
        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);
        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);
        paintBrush.SmoothToLocation(largeEnoughSecondLocation, m_settings);
        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushSmoothLocationTestFixture, SmoothToLocationSecondMovementDoesNotIncludeFirstCircle)
    {
        // When smoothing, the first SmoothToLocation call should just contain a single brush stamp at the passed-in location.
        // The second SmoothToLocation call should contain brush stamps from the first location to the second, but should NOT
        // have a second brush stamp at the first location. Ex:
        //     O            <- first SmoothToLocation
        //     -OOO         <- second SmoothToLocation
        // If the Distance % is anything less than 100% in the paint brush settings, the Os will overlap.
        // We'll set it to 100% just to make it obvious that we've gotten the correct result.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
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

        // We expect to get two OnSmooth calls.
        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=](const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // On the first SmoothToLocation, we expect to get a dirtyArea that exactly fits our first paint brush stamp.
                    const AZ::Aabb expectedFirstDirtyArea = AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, TestBrushRadius);
                    EXPECT_THAT(dirtyArea, IsClose(expectedFirstDirtyArea));
                });

        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);

        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=](const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // On the second SmoothToLocation, we expect the dirtyArea to only contain the next 3 paint brush stamps,
                    // but not the first one.
                    const AZ::Vector3 stampIncrement = AZ::Vector3(TestBrushSize, 0.0f, 0.0f);
                    AZ::Aabb expectedSecondDirtyArea = AZ::Aabb::CreateCenterRadius(TestBrushCenter2d + stampIncrement, TestBrushRadius);
                    expectedSecondDirtyArea.AddAabb(
                        AZ::Aabb::CreateCenterRadius(TestBrushCenter2d + (3.0f * stampIncrement), TestBrushRadius));
                    EXPECT_THAT(dirtyArea, IsClose(expectedSecondDirtyArea));
                });

        paintBrush.SmoothToLocation(secondLocation, m_settings);

        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushSmoothLocationTestFixture, EyedropperDoesNotAffectSmoothToLocation)
    {
        // When smoothing, we should be able to call UseEyedropper at any arbitrary location without affecting the current
        // state of SmoothToLocation.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
        ::testing::NiceMock<MockPaintBrushNotificationBusHandler> mockHandler(EntityComponentIdPair);

        // Set the brush radius to 1 meter (diameter is 2 meters)
        const float TestBrushRadius = 1.0f;
        const float TestBrushSize = TestBrushRadius * 2.0f;
        m_settings.SetSize(TestBrushSize);

        // Choose a second brush center location that's 2 full brush stamps away in the X direction only.
        AZ::Vector3 secondLocation = TestBrushCenter + AZ::Vector3(TestBrushSize * 2.0f, 0.0f, 0.0f);

        // We expect to get two OnSmooth calls.
        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=](const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // We expect that the Y value for our dirtyArea won't be changed even though we'll call UseEyedropper
                    // with a large Y value in-between the two paint calls.
                    EXPECT_NEAR(dirtyArea.GetMin().GetY(), TestBrushCenter.GetY() - TestBrushRadius, 0.01f);
                    EXPECT_NEAR(dirtyArea.GetMax().GetY(), TestBrushCenter.GetY() + TestBrushRadius, 0.01f);
                });

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);

        // Call UseEyeDropper with a large Y value so that we can easily detect if it affected our SmoothToLocation calls.
        [[maybe_unused]] AZ::Color color = paintBrush.UseEyedropper(TestBrushCenter + AZ::Vector3(0.0f, 1000.0f, 0.0f));

        paintBrush.SmoothToLocation(secondLocation, m_settings);

        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }

    TEST_F(PaintBrushSmoothLocationTestFixture, ResetBrushStrokeTrackingWorksCorrectly)
    {
        // If ResetBrushStrokeTracking is called in-between two calls to SmoothtToLocation within a brush stroke,
        // there should be a discontinuity between the two locations as if the brush has been picked up and put back down.
        // i.e. Instead of 'OOOOOO' between two locations it should create 'O     O'.
        // This is typically used for handling things like leaving the edge of the image at one location and coming back onto
        // the image at a different location.

        AzToolsFramework::PaintBrush paintBrush(EntityComponentIdPair);
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

        // We expect to get two OnSmooth calls.
        EXPECT_CALL(mockHandler, OnSmooth(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(2);

        paintBrush.BeginPaintMode();
        paintBrush.BeginBrushStroke(m_settings);

        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=](const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // On the first PaintToLocation, we expect to get a dirtyArea that exactly fits our first paint brush stamp.
                    const AZ::Aabb expectedFirstDirtyArea = AZ::Aabb::CreateCenterRadius(TestBrushCenter2d, TestBrushRadius);
                    EXPECT_THAT(dirtyArea, IsClose(expectedFirstDirtyArea));
                });

        paintBrush.SmoothToLocation(TestBrushCenter, m_settings);

        // Reset the brush stroke tracking, so that the next location will look like the start of a stroke again.
        paintBrush.ResetBrushStrokeTracking();

        ON_CALL(mockHandler, OnSmooth)
            .WillByDefault(
                [=](const AZ::Aabb& dirtyArea,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::ValueLookupFn& valueLookupFn,
                    [[maybe_unused]] AZStd::span<const AZ::Vector3> valuePointOffsets,
                    [[maybe_unused]] AzToolsFramework::PaintBrushNotifications::SmoothFn& smoothFn)
                {
                    // On the second PaintToLocation, we expect to get a dirtyArea that exactly fits our second paint brush stamp.
                    // It should *not* include any of the space bewteen the first and the second brush stamp.
                    const AZ::Vector3 secondLocation2d(AZ::Vector2(secondLocation), 0.0f);
                    const AZ::Aabb expectedSecondDirtyArea = AZ::Aabb::CreateCenterRadius(secondLocation2d, TestBrushRadius);
                    EXPECT_THAT(dirtyArea, IsClose(expectedSecondDirtyArea));
                });

        paintBrush.SmoothToLocation(secondLocation, m_settings);

        paintBrush.EndBrushStroke();
        paintBrush.EndPaintMode();
    }
} // namespace UnitTest
