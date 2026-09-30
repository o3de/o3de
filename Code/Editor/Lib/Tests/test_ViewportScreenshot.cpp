/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Core/ViewportScreenshotCapture.h>
#include <Core/ViewportScreenshotUtils.h>

namespace UnitTest
{
    using EditorViewportScreenshot::MakeCaptureOutputPath;
    using EditorViewportScreenshot::ResolveCaptureResolution;

    // Mirrors the cap the editor passes in for viewport screenshots.
    constexpr uint32_t MaxTestDimension = 8192;

    // A valid request passes through unchanged (1920x1080 in, 1920x1080 out).
    TEST(ViewportScreenshotResolutionTests, RequestedResolutionIsUsed)
    {
        const auto resolution = ResolveCaptureResolution(1920, 1080, 800, 600, MaxTestDimension);

        ASSERT_TRUE(resolution.IsSuccess());
        EXPECT_EQ(resolution.GetValue().m_width, 1920u);
        EXPECT_EQ(resolution.GetValue().m_height, 1080u);
    }

    // A 0x0 request means "use the current viewport size".
    TEST(ViewportScreenshotResolutionTests, NonPositiveRequestFallsBackToViewportSize)
    {
        const auto resolution = ResolveCaptureResolution(0, 0, 800, 600, MaxTestDimension);

        ASSERT_TRUE(resolution.IsSuccess());
        EXPECT_EQ(resolution.GetValue().m_width, 800u);
        EXPECT_EQ(resolution.GetValue().m_height, 600u);
    }

    // 0x0 requested and 0x0 viewport fails instead of producing a zero-size capture.
    TEST(ViewportScreenshotResolutionTests, UnusableViewportSizeIsRejected)
    {
        const auto resolution = ResolveCaptureResolution(0, 0, 0, 0, MaxTestDimension);

        EXPECT_FALSE(resolution.IsSuccess());
        EXPECT_FALSE(resolution.GetError().empty());
    }

    // A width over the cap fails.
    TEST(ViewportScreenshotResolutionTests, WidthAboveMaximumIsRejected)
    {
        const auto resolution = ResolveCaptureResolution(static_cast<int>(MaxTestDimension) + 1, 1080, 800, 600, MaxTestDimension);

        EXPECT_FALSE(resolution.IsSuccess());
        EXPECT_FALSE(resolution.GetError().empty());
    }

    // A height over the cap fails (the other half of the ||).
    TEST(ViewportScreenshotResolutionTests, HeightAboveMaximumIsRejected)
    {
        const auto resolution = ResolveCaptureResolution(1920, static_cast<int>(MaxTestDimension) + 1, 800, 600, MaxTestDimension);

        EXPECT_FALSE(resolution.IsSuccess());
    }

    // Exactly at the cap is legal, so the bound is > rather than >=.
    TEST(ViewportScreenshotResolutionTests, ResolutionAtMaximumIsAccepted)
    {
        const auto resolution =
            ResolveCaptureResolution(static_cast<int>(MaxTestDimension), static_cast<int>(MaxTestDimension), 800, 600, MaxTestDimension);

        ASSERT_TRUE(resolution.IsSuccess());
        EXPECT_EQ(resolution.GetValue().m_width, MaxTestDimension);
        EXPECT_EQ(resolution.GetValue().m_height, MaxTestDimension);
    }

    // Captures are named Snapshot_<timestamp>_<width>x<height>_<sequence>.png.
    TEST(ViewportScreenshotOutputPathTests, PathContainsResolutionAndSequence)
    {
        const AZStd::string path = MakeCaptureOutputPath("/project", "20261004_120000", 1920, 1080, 7);

        EXPECT_STREQ(path.c_str(), "/project/user/Screenshots/Snapshot_20261004_120000_1920x1080_7.png");
    }

    // Begin() returns false and leaves the capture inactive on a null scene/view or zero size.
    TEST(ViewportScreenshotCaptureTests, BeginWithoutSceneOrViewFails)
    {
        ViewportScreenshotCapture capture(nullptr, "screenshot.png", false);

        EXPECT_FALSE(capture.Begin(AZ::RPI::ScenePtr(), AZ::RPI::ViewPtr(), 1920, 1080));
        EXPECT_FALSE(capture.Begin(AZ::RPI::ScenePtr(), AZ::RPI::ViewPtr(), 0, 0));
        EXPECT_FALSE(capture.IsCapturing());
    }

    // Destroying a capture that never acquired a pipeline does not crash.
    TEST(ViewportScreenshotCaptureTests, FailedBeginIsTornDownOnDestruction)
    {
        ViewportScreenshotCapture capture(nullptr, "screenshot.png", true);

        EXPECT_FALSE(capture.Begin(AZ::RPI::ScenePtr(), AZ::RPI::ViewPtr(), 100, 100));
    }
} // namespace UnitTest
