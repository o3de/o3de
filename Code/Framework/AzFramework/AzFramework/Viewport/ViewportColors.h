/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

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
        extern const AZ::Color DeselectedColor;

        /// Color to use for a selected object
        extern const AZ::Color SelectedColor;

        /// Color to use when hovering
        extern const AZ::Color HoverColor;

        /// Color to use for wireframe
        extern const AZ::Color WireColor;

        /// Color to use for locked
        extern const AZ::Color LockColor;

        /// Color to use for hidden
        extern const AZ::Color HiddenColor;

        /// Color to use for x-axis
        extern const AZ::Color XAxisColor;

        /// Color to use for y-axis
        extern const AZ::Color YAxisColor;

        /// Color to use for z-axis
        extern const AZ::Color ZAxisColor;

        /// Color used by QuadBillboard Manipulator View.
        extern const AZ::Color DefaultManipulatorHandleColor;

    } // namespace ViewportColors
} // namespace AzFramework
