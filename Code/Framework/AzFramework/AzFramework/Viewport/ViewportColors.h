/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/AzFrameworkAPI.h>

namespace AZ
{
    class Color;
}

namespace AzFramework
{
    /// Various colors used by the editor and shared between objects
    namespace ViewportColors
    {
        /// Color to use for a deselected object
        AZF_API extern const AZ::Color DeselectedColor;

        /// Color to use for a selected object
        AZF_API extern const AZ::Color SelectedColor;

        /// Color to use when hovering
        AZF_API extern const AZ::Color HoverColor;

        /// Color to use for wireframe
        AZF_API extern const AZ::Color WireColor;

        /// Color to use for locked
        AZF_API extern const AZ::Color LockColor;

        /// Color to use for hidden
        AZF_API extern const AZ::Color HiddenColor;

        /// Color to use for x-axis
        AZF_API extern const AZ::Color XAxisColor;

        /// Color to use for y-axis
        AZF_API extern const AZ::Color YAxisColor;

        /// Color to use for z-axis
        AZF_API extern const AZ::Color ZAxisColor;

        /// Color used by QuadBillboard Manipulator View.
        AZF_API extern const AZ::Color DefaultManipulatorHandleColor;

    } // namespace ViewportColors
} // namespace AzFramework
