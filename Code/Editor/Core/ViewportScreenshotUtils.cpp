/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Core/ViewportScreenshotUtils.h>

#include <AzCore/Casting/numeric_cast.h>

namespace EditorViewportScreenshot
{
    AZ::Outcome<CaptureResolution, AZStd::string> ResolveCaptureResolution(
        int requestedWidth, int requestedHeight, uint32_t viewportWidth, uint32_t viewportHeight, uint32_t maxDimension)
    {
        int width = requestedWidth;
        int height = requestedHeight;

        // A non-positive request means "use the current viewport size".
        if (width <= 0 || height <= 0)
        {
            width = aznumeric_cast<int>(viewportWidth);
            height = aznumeric_cast<int>(viewportHeight);
        }

        if (width <= 0 || height <= 0)
        {
            return AZ::Failure(AZStd::string::format("invalid resolution %ix%i", width, height));
        }

        // Hard cap on one dimension, mirroring CustomResolutionDlg::MAX_RES.
        if (aznumeric_cast<uint32_t>(width) > maxDimension || aznumeric_cast<uint32_t>(height) > maxDimension)
        {
            return AZ::Failure(AZStd::string::format("resolution %ix%i exceeds the %u maximum", width, height, maxDimension));
        }

        return AZ::Success(CaptureResolution{ aznumeric_cast<uint32_t>(width), aznumeric_cast<uint32_t>(height) });
    }

    AZStd::string MakeCaptureOutputPath(
        AZStd::string_view projectPath, AZStd::string_view timestamp, uint32_t width, uint32_t height, uint32_t sequence)
    {
        AZStd::string outputPath(projectPath.data(), projectPath.size());
        outputPath += "/user/Screenshots/Snapshot_";
        outputPath.append(timestamp.data(), timestamp.size());
        outputPath += AZStd::string::format("_%ux%u_%u.png", width, height, sequence);
        return outputPath;
    }
} // namespace EditorViewportScreenshot
