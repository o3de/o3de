/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
