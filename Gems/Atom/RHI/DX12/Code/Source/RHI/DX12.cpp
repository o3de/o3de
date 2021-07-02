/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        DXGI_FORMAT MakeSRGB(_In_ DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

            case DXGI_FORMAT_BC1_UNORM:
                return DXGI_FORMAT_BC1_UNORM_SRGB;

            case DXGI_FORMAT_BC2_UNORM:
                return DXGI_FORMAT_BC2_UNORM_SRGB;

            case DXGI_FORMAT_BC3_UNORM:
                return DXGI_FORMAT_BC3_UNORM_SRGB;

            case DXGI_FORMAT_B8G8R8A8_UNORM:
                return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

            case DXGI_FORMAT_B8G8R8X8_UNORM:
                return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

            case DXGI_FORMAT_BC7_UNORM:
                return DXGI_FORMAT_BC7_UNORM_SRGB;

            default:
                return format;
            }
        }

        DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat)
        {
            switch (defaultFormat)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return DXGI_FORMAT_R8G8B8A8_TYPELESS;

            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8A8_TYPELESS;

            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8X8_TYPELESS;

                // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_R32G8X24_TYPELESS;

                // No Stencil
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_R32_TYPELESS;

                // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_R24G8_TYPELESS;

            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
                return DXGI_FORMAT_BC1_TYPELESS;

            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
                return DXGI_FORMAT_BC2_TYPELESS;

            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                return DXGI_FORMAT_BC3_TYPELESS;

            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
                return DXGI_FORMAT_BC4_TYPELESS;

            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
                return DXGI_FORMAT_BC5_TYPELESS;

                // 16-bit Z w/o Stencil
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
                return DXGI_FORMAT_R16_TYPELESS;

            default:
                return defaultFormat;
            }
        }

        DXGI_FORMAT GetUAVFormat(DXGI_FORMAT defaultFormat)
        {
            switch (defaultFormat)
            {
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8A8_UNORM;

            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                return DXGI_FORMAT_B8G8R8X8_UNORM;

            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;

#if defined(AZ_DEBUG_BUILD)
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            case DXGI_FORMAT_D16_UNORM:

                AZ_Assert(false, "Requested a UAV format for a depth stencil format.");
#endif

            default:
                return defaultFormat;
            }
        }

        DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat)
        {
            switch (defaultFormat)
            {
                // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

                // No Stencil
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_D32_FLOAT;

                // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;

                // 16-bit Z w/o Stencil
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
                return DXGI_FORMAT_D16_UNORM;

            default:
                return defaultFormat;
            }
        }

        DXGI_FORMAT GetSRVFormat(DXGI_FORMAT defaultFormat)
        {
            switch (defaultFormat)
            {
                // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

                // No Stencil
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;

                // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

                // 16-bit Z w/o Stencil
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
                return DXGI_FORMAT_R16_UNORM;

            default:
                return defaultFormat;
            }
        }

        DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat)
        {
            switch (defaultFormat)
            {
                // 32-bit Z w/ Stencil
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
                return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

                // 24-bit Z
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
                return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

            default:
                return DXGI_FORMAT_UNKNOWN;
            }
        }
    }
}
