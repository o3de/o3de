/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <SandboxAPI.h>

//! Renderer-independent helpers for the viewport screenshot feature, kept free of Atom and Qt so the
//! resolution policy and output naming can be unit tested without a render pipeline.
namespace EditorViewportScreenshot
{
    //! A validated capture resolution, in pixels.
    struct CaptureResolution
    {
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };

    //! Resolves a requested capture resolution, falling back to the viewport size when the request is
    //! non-positive, and enforcing the maximum dimension so any entry path bounds VRAM allocation.
    //! @return the resolution to capture at, or a message describing why the request is unusable.
    SANDBOX_API AZ::Outcome<CaptureResolution, AZStd::string> ResolveCaptureResolution(
        int requestedWidth, int requestedHeight, uint32_t viewportWidth, uint32_t viewportHeight, uint32_t maxDimension);

    //! Builds the output path for a capture; the monotonically increasing sequence keeps captures taken
    //! within the same second from overwriting each other.
    SANDBOX_API AZStd::string MakeCaptureOutputPath(
        AZStd::string_view projectPath, AZStd::string_view timestamp, uint32_t width, uint32_t height, uint32_t sequence);
} // namespace EditorViewportScreenshot
