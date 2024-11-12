/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//           |                    name                                     
//           |   RHI::Format        |      wgpu::VertexFormat  |
#define RHIWGPU_EXPAND_FOR_FORMATS(_Func)               \
        _Func(R8G8_UINT,                Uint8x2)        \
        _Func(R8G8B8A8_UINT,            Uint8x4)        \
        _Func(R8G8_SINT,                Sint8x2)        \
        _Func(R8G8B8A8_SINT,            Sint8x4)        \
        _Func(R8G8_UNORM,               Unorm8x2)       \
        _Func(R8G8B8A8_UNORM,           Unorm8x4)       \
        _Func(B8G8R8A8_UNORM,           Unorm8x4)       \
        _Func(R8G8_SNORM,               Snorm8x2)       \
        _Func(R8G8B8A8_SNORM,           Snorm8x4)       \
        _Func(R16G16_UINT,              Uint16x2)       \
        _Func(R16G16B16A16_UINT,        Uint16x4)       \
        _Func(R16G16_SINT,              Sint16x2)       \
        _Func(R16G16B16A16_SINT,        Sint16x4)       \
        _Func(R16G16_UNORM,             Unorm16x2)      \
        _Func(R16G16B16A16_UNORM,       Unorm16x4)      \
        _Func(R16G16_SNORM,             Snorm16x2)      \
        _Func(R16G16B16A16_SNORM,       Snorm16x4)      \
        _Func(R16G16_FLOAT,             Float16x2)      \
        _Func(R16G16B16A16_FLOAT,       Float16x4)      \
        _Func(R32_FLOAT,                Float32)        \
        _Func(R32G32_FLOAT,             Float32x2)      \
        _Func(R32G32B32_FLOAT,          Float32x3)      \
        _Func(R32G32B32A32_FLOAT,       Float32x4)      \
        _Func(R32_UINT,                 Uint32)         \
        _Func(R32G32_UINT,              Uint32x2)       \
        _Func(R32G32B32_UINT,           Uint32x3)       \
        _Func(R32G32B32A32_UINT,        Uint32x4)       \
        _Func(R32_SINT,                 Sint32)         \
        _Func(R32G32_SINT,              Sint32x2)       \
        _Func(R32G32B32_SINT,           Sint32x3)       \
        _Func(R32G32B32A32_SINT,        Sint32x4)       \
        _Func(R10G10B10A2_UNORM,        Unorm10_10_10_2)
