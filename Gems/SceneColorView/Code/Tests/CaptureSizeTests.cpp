/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include <SceneColorView/CaptureSize.h>
#include <cstdint>
#include <limits>

using SceneColorView::CalculateCaptureSize;
static_assert(CalculateCaptureSize(1920, 1080, 1).m_width == 1920);
static_assert(CalculateCaptureSize(1920, 1080, 2).m_width == 960);
static_assert(CalculateCaptureSize(1920, 1080, 4).m_height == 270);
static_assert(CalculateCaptureSize(1919, 1079, 2).m_width == 960);
static_assert(CalculateCaptureSize(1919, 1079, 4).m_height == 270);
static_assert(CalculateCaptureSize(0, 0, 4).m_width == 1);
static_assert(CalculateCaptureSize(1, 1, 4).m_height == 1);
static_assert(CalculateCaptureSize(100, 100, 0).m_width == 50);
static_assert(CalculateCaptureSize(100, 100, 99).m_width == 50);
static_assert(CalculateCaptureSize(std::numeric_limits<uint32_t>::max(), 1, 2).m_width == 2147483648u);
int main() { return 0; }
