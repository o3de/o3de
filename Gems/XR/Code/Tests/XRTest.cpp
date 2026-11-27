/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <XR/XRUtils.h>
#include <cmath>

TEST(XRProjection, AsymmetricFrustumProjectsToClipBounds)
{
    constexpr float left = -0.6f;
    constexpr float right = 0.4f;
    constexpr float bottom = -0.5f;
    constexpr float top = 0.7f;
    constexpr float nearDistance = 0.1f;
    constexpr float farDistance = 100.0f;
    const auto projection = XR::CreateStereoscopicProjection(left, right, bottom, top, nearDistance, farDistance, false);

    const auto lowerLeft = projection * AZ::Vector4(std::tan(left) * nearDistance, std::tan(bottom) * nearDistance, -nearDistance, 1.0f);
    const auto upperRight = projection * AZ::Vector4(std::tan(right) * nearDistance, std::tan(top) * nearDistance, -nearDistance, 1.0f);
    EXPECT_TRUE((lowerLeft / lowerLeft.GetW()).IsClose(AZ::Vector4(-1.0f, -1.0f, -1.0f, 1.0f)));
    EXPECT_TRUE((upperRight / upperRight.GetW()).IsClose(AZ::Vector4(1.0f, 1.0f, -1.0f, 1.0f)));
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
