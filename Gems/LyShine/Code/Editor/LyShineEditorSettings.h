/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AzCore/base.h"
#include "ViewportInteraction.h"

namespace LyShine
{
    inline constexpr AZStd::string_view InteractionModeSetting = "/O3DE/Preferences/Editor/LyShine/InteractionMode";
    inline constexpr AZStd::string_view CoordinateSystemSetting = "/O3DE/Preferences/Editor/LyShine/CoordinateSystem";

    ViewportInteraction::InteractionMode GetInteractionMode();
    void SetInteractionMode(ViewportInteraction::InteractionMode mode);

    ViewportInteraction::CoordinateSystem GetCoordinateSystem();
    void SetCoordinateSystem(ViewportInteraction::CoordinateSystem length);
} // namespace LyShine
