/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//           |                    name                                      |     Aspect Flags      |
//           |   RHI::Format        |      wgpu::TextureFormat              |Color | Depth | Stencil|
#define RHIWGPU_EXPAND_FOR_FORMATS(_Func) \
        _Func(R32G32B32A32_FLOAT,       RGBA32Float,                            1,      0,      0) \
        _Func(R32G32B32A32_UINT,        RGBA32Uint,                             1,      0,      0) \
        _Func(R32G32B32A32_SINT,        RGBA32Sint,                             1,      0,      0) \
        _Func(R16G16B16A16_FLOAT,       RGBA16Float,                            1,      0,      0) \
        _Func(R16G16B16A16_UNORM,       RGBA16Unorm,                            1,      0,      0) \
        _Func(R16G16B16A16_UINT,        RGBA16Uint,                             1,      0,      0) \
        _Func(R16G16B16A16_SNORM,       RGBA16Snorm,                            1,      0,      0) \
        _Func(R16G16B16A16_SINT,        RGBA16Sint,                             1,      0,      0) \
        _Func(R32G32_FLOAT,             RG32Float,                              1,      0,      0) \
        _Func(R32G32_UINT,              RG32Uint,                               1,      0,      0) \
        _Func(R32G32_SINT,              RG32Sint,                               1,      0,      0) \
        _Func(D32_FLOAT_S8X24_UINT,     Depth32FloatStencil8,                   0,      1,      1) \
        _Func(R10G10B10A2_UNORM,        RGB10A2Unorm,                           1,      0,      0) \
        _Func(R10G10B10A2_UINT,         RGB10A2Uint,                            1,      0,      0) \
        _Func(R11G11B10_FLOAT,          RG11B10Ufloat,                          1,      0,      0) \
        _Func(R8G8B8A8_UNORM,           RGBA8Unorm,                             1,      0,      0) \
        _Func(R8G8B8A8_UNORM_SRGB,      RGBA8UnormSrgb,                         1,      0,      0) \
        _Func(R8G8B8A8_UINT,            RGBA8Uint,                              1,      0,      0) \
        _Func(R8G8B8A8_SNORM,           RGBA8Snorm,                             1,      0,      0) \
        _Func(R8G8B8A8_SINT,            RGBA8Sint,                              1,      0,      0) \
        _Func(R16G16_FLOAT,             RG16Float,                              1,      0,      0) \
        _Func(R16G16_UNORM,             RG16Unorm,                              1,      0,      0) \
        _Func(R16G16_UINT,              RG16Uint,                               1,      0,      0) \
        _Func(R16G16_SNORM,             RG16Snorm,                              1,      0,      0) \
        _Func(R16G16_SINT,              RG16Sint,                               1,      0,      0) \
        _Func(D32_FLOAT,                Depth32Float,                           0,      1,      0) \
        _Func(R32_FLOAT,                R32Float,                               1,      0,      0) \
        _Func(R32_UINT,                 R32Uint,                                1,      0,      0) \
        _Func(R32_SINT,                 R32Sint,                                1,      0,      0) \
        _Func(D24_UNORM_S8_UINT,        Depth24PlusStencil8,                    0,      1,      1) \
        _Func(R8G8_UNORM,               RG8Unorm,                               1,      0,      0) \
        _Func(R8G8_UINT,                RG8Uint,                                1,      0,      0) \
        _Func(R8G8_SNORM,               RG8Snorm,                               1,      0,      0) \
        _Func(R8G8_SINT,                RG8Sint,                                1,      0,      0) \
        _Func(R16_FLOAT,                R16Float,                               1,      0,      0) \
        _Func(D16_UNORM,                Depth16Unorm,                           0,      1,      0) \
        _Func(R16_UNORM,                R16Unorm,                               1,      0,      0) \
        _Func(R16_UINT,                 R16Uint,                                1,      0,      0) \
        _Func(R16_SNORM,                R16Snorm,                               1,      0,      0) \
        _Func(R16_SINT,                 R16Sint,                                1,      0,      0) \
        _Func(R8_UNORM,                 R8Unorm,                                1,      0,      0) \
        _Func(R8_UINT,                  R8Uint,                                 1,      0,      0) \
        _Func(R8_SNORM,                 R8Snorm,                                1,      0,      0) \
        _Func(R8_SINT,                  R8Sint,                                 1,      0,      0) \
        _Func(R9G9B9E5_SHAREDEXP,       RGB9E5Ufloat,                           1,      0,      0) \
        _Func(R8G8_B8G8_UNORM,          R8BG8Biplanar422Unorm,                  1,      0,      0) \
        _Func(BC1_UNORM,                BC1RGBAUnorm,                           1,      0,      0) \
        _Func(BC1_UNORM_SRGB,           BC1RGBAUnormSrgb,                       1,      0,      0) \
        _Func(BC2_UNORM,                BC2RGBAUnorm,                           1,      0,      0) \
        _Func(BC2_UNORM_SRGB,           BC2RGBAUnormSrgb,                       1,      0,      0) \
        _Func(BC3_UNORM,                BC3RGBAUnorm,                           1,      0,      0) \
        _Func(BC3_UNORM_SRGB,           BC3RGBAUnormSrgb,                       1,      0,      0) \
        _Func(BC4_UNORM,                BC4RUnorm,                              1,      0,      0) \
        _Func(BC4_SNORM,                BC4RSnorm,                              1,      0,      0) \
        _Func(BC5_UNORM,                BC5RGUnorm,                             1,      0,      0) \
        _Func(BC5_SNORM,                BC5RGSnorm,                             1,      0,      0) \
        _Func(B8G8R8A8_UNORM,           BGRA8Unorm,                             1,      0,      0) \
        _Func(B8G8R8A8_UNORM_SRGB,      BGRA8UnormSrgb,                         1,      0,      0) \
        _Func(BC6H_UF16,                BC6HRGBUfloat,                          1,      0,      0) \
        _Func(BC6H_SF16,                BC6HRGBFloat,                           1,      0,      0) \
        _Func(BC7_UNORM,                BC7RGBAUnorm,                           1,      0,      0) \
        _Func(BC7_UNORM_SRGB,           BC7RGBAUnormSrgb,                       1,      0,      0) \
        _Func(EAC_R11_UNORM,            EACR11Unorm,                            1,      0,      0) \
        _Func(EAC_R11_SNORM,            EACR11Snorm,                            1,      0,      0) \
        _Func(EAC_RG11_UNORM,           EACRG11Unorm,                           1,      0,      0) \
        _Func(EAC_RG11_SNORM,           EACRG11Snorm,                           1,      0,      0) \
        _Func(ETC2_UNORM,               ETC2RGB8Unorm,                          1,      0,      0) \
        _Func(ETC2_UNORM_SRGB,          ETC2RGB8UnormSrgb,                      1,      0,      0) \
        _Func(ETC2A1_UNORM,             ETC2RGB8A1Unorm,                        1,      0,      0) \
        _Func(ETC2A1_UNORM_SRGB,        ETC2RGB8A1UnormSrgb,                    1,      0,      0) \
        _Func(ETC2A_UNORM,              ETC2RGBA8Unorm,                         1,      0,      0) \
        _Func(ETC2A_UNORM_SRGB,         ETC2RGBA8UnormSrgb,                     1,      0,      0) \
        _Func(ASTC_4x4_UNORM,           ASTC4x4Unorm,                           1,      0,      0) \
        _Func(ASTC_4x4_UNORM_SRGB,      ASTC4x4UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_5x4_UNORM,           ASTC5x4Unorm,                           1,      0,      0) \
        _Func(ASTC_5x4_UNORM_SRGB,      ASTC5x4UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_5x5_UNORM,           ASTC5x5Unorm,                           1,      0,      0) \
        _Func(ASTC_5x5_UNORM_SRGB,      ASTC5x5UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_6x5_UNORM,           ASTC6x5Unorm,                           1,      0,      0) \
        _Func(ASTC_6x5_UNORM_SRGB,      ASTC6x5UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_6x6_UNORM,           ASTC6x6Unorm,                           1,      0,      0) \
        _Func(ASTC_6x6_UNORM_SRGB,      ASTC6x6UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_8x5_UNORM,           ASTC8x5Unorm,                           1,      0,      0) \
        _Func(ASTC_8x5_UNORM_SRGB,      ASTC8x5UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_8x6_UNORM,           ASTC8x6Unorm,                           1,      0,      0) \
        _Func(ASTC_8x6_UNORM_SRGB,      ASTC8x6UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_8x8_UNORM,           ASTC8x8Unorm,                           1,      0,      0) \
        _Func(ASTC_8x8_UNORM_SRGB,      ASTC8x8UnormSrgb,                       1,      0,      0) \
        _Func(ASTC_10x5_UNORM,          ASTC10x5Unorm,                          1,      0,      0) \
        _Func(ASTC_10x5_UNORM_SRGB,     ASTC10x5UnormSrgb,                      1,      0,      0) \
        _Func(ASTC_10x6_UNORM,          ASTC10x6Unorm,                          1,      0,      0) \
        _Func(ASTC_10x6_UNORM_SRGB,     ASTC10x6UnormSrgb,                      1,      0,      0) \
        _Func(ASTC_10x8_UNORM,          ASTC10x8Unorm,                          1,      0,      0) \
        _Func(ASTC_10x8_UNORM_SRGB,     ASTC10x8UnormSrgb,                      1,      0,      0) \
        _Func(ASTC_10x10_UNORM,         ASTC10x10Unorm,                         1,      0,      0) \
        _Func(ASTC_10x10_UNORM_SRGB,    ASTC10x10UnormSrgb,                     1,      0,      0) \
        _Func(ASTC_12x10_UNORM,         ASTC12x10Unorm,                         1,      0,      0) \
        _Func(ASTC_12x10_UNORM_SRGB,    ASTC12x10UnormSrgb,                     1,      0,      0) \
        _Func(ASTC_12x12_UNORM,         ASTC12x12Unorm,                         1,      0,      0) \
        _Func(ASTC_12x12_UNORM_SRGB,    ASTC12x12UnormSrgb,                     1,      0,      0)
