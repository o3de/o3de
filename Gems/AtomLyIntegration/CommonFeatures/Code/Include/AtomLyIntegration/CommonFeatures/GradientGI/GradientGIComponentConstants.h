/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    namespace Render
    {
        inline constexpr AZ::TypeId GradientGIComponentTypeId{ "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}" };
        inline constexpr AZ::TypeId EditorGradientGIComponentTypeId{ "{B2C3D4E5-F6A7-8901-BCDE-F12345678901}" };

        // =====================================================================
        // Update Mode
        // =====================================================================

        //! Controls whether the gradient cubemap is generated on CPU or GPU.
        enum class GradientGIUpdateMode : uint8_t
        {
            //! CPU-generated StreamingImage -- mobile-safe, one rebuild per change.
            Static  = 0,

            //! GPU compute pass writing an AttachmentImage every frame.
            //! Requires compute shader UAV support (DX12 / Vulkan desktop).
            //! Falls back to Static automatically on unsupported platforms.
            Dynamic = 1,
        };

    } // namespace Render
} // namespace AZ
