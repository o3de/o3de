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

        //! Generation backend. Both backends update when inputs change.
        enum class GradientGIUpdateMode : uint8_t
        {
            //! CPU-generated StreamingImage.
            Static = 0,

            //! GPU-generated AttachmentImage. Falls back to Static if writable cubemap allocation is unavailable.
            Dynamic = 1,
        };

        //! Texture mapping codes shared with GradientGICubemap.azsl. Preserve their numeric values.
        enum class GradientGITextureMapping : uint8_t
        {
            Tiled = 0, //! 2D texture repeated per cube face using the face's UV basis.
            Stretched = 1, //! 2D texture wrapped over the sphere (equirectangular).
            Cube = 2, //! Cubemap sampled by direction.
        };

        //! Blend codes shared with GradientGICubemap.azsl. Preserve their numeric values.
        enum class GradientGIBlendMode : uint8_t
        {
            Multiply = 0,
            Add = 1,
            Screen = 2,
            Overlay = 3,
            Replace = 4, //! Use the texture without the base color.
        };

    } // namespace Render
} // namespace AZ
