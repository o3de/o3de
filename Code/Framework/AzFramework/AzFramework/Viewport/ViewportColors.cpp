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

#include "ViewportColors.h"

#include <AzCore/Math/Color.h>

namespace AzFramework
{
    /// Various colors used by the editor and shared between objects
    namespace ViewportColors
    {
        /// Color to use for a deselected object
        const AZ::Color DeselectedColor(1.0f, 1.0f, 0.78f, 0.4f);

        /// Color to use for a selected object
        const AZ::Color SelectedColor(1.0, 1.0f, 0.0f, 0.9f);

        /// Color to use when hovering
        const AZ::Color HoverColor(1.0f, 1.0f, 0.0f, 0.9f);

        /// Color to use for wireframe
        const AZ::Color WireColor(1.0f, 1.0f, 0.78f, 0.5f);

        /// Color to use for locked
        const AZ::Color LockColor(0.5f, 0.5f, 0.5f, 1.0f);

        /// Color to use for hidden
        const AZ::Color HiddenColor(1.0f, 1.0f, 1.0f, 1.0f);

        /// Color to use for x-axis
        const AZ::Color XAxisColor(1.0f, 0.0f, 0.0f, 1.0f);

        /// Color to use for y-axis
        const AZ::Color YAxisColor(0.0f, 1.0f, 0.0f, 1.0f);

        /// Color to use for z-axis
        const AZ::Color ZAxisColor(0.0f, 0.0f, 1.0f, 1.0f);

        /// Color used by QuadBillboard Manipulator View.
        const AZ::Color DefaultManipulatorHandleColor(0.06275f, 0.1647f, 0.1647f, 1.0f);
        
    } // namespace ViewportColors
} // namespace AzFramework