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

        // =====================================================================
        // Texture Layer Mapping / Blend (GPU/Dynamic-mode texturing)
        // =====================================================================

        //! How a detail/specular texture is mapped onto the gradient cubemap.
        //! Codes are forwarded to the compute shader as plain uints, so the values
        //! must stay in sync with GradientGICubemap.azsl.
        enum class GradientGITextureMapping : uint8_t
        {
            Tiled     = 0,  //! 2D texture repeated per cube face using the face's UV basis.
            Stretched = 1,  //! 2D texture wrapped over the sphere (equirectangular).
            Cube      = 2,  //! Authored cubemap sampled by direction (added in a later phase).
        };

        //! How a texture layer combines with the color beneath it.
        enum class GradientGIBlendMode : uint8_t
        {
            Multiply = 0,
            Add      = 1,
            Screen   = 2,
            Overlay  = 3,
            Replace  = 4,  //! Use the texture alone, ignoring the base (specular layer).
        };

    } // namespace Render
} // namespace AZ
