/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

// Texture formats
enum ETEX_Format : AZ::u8
{
    eTF_Unknown = 0,
    eTF_R8G8B8A8S,
    eTF_R8G8B8A8 = 2, // may be saved into file

    eTF_A8 = 4,
    eTF_R8,
    eTF_R8S,
    eTF_R16,
    eTF_R16F,
    eTF_R32F,
    eTF_R8G8,
    eTF_R8G8S,
    eTF_R16G16,
    eTF_R16G16S,
    eTF_R16G16F,
    eTF_R11G11B10F,
    eTF_R10G10B10A2,
    eTF_R16G16B16A16,
    eTF_R16G16B16A16S,
    eTF_R16G16B16A16F,
    eTF_R32G32B32A32F,

    eTF_CTX1,
    eTF_BC1 = 22, // may be saved into file
    eTF_BC2 = 23, // may be saved into file
    eTF_BC3 = 24, // may be saved into file
    eTF_BC4U,     // 3Dc+
    eTF_BC4S,
    eTF_BC5U,     // 3Dc
    eTF_BC5S,
    eTF_BC6UH,
    eTF_BC6SH,
    eTF_BC7,
    eTF_R9G9B9E5,

    // hardware depth buffers
    eTF_D16,
    eTF_D24S8,
    eTF_D32F,
    eTF_D32FS8,

    // only available as hardware format under DX11.1 with DXGI 1.2
    eTF_B5G6R5,
    eTF_B5G5R5,
    eTF_B4G4R4A4,

    // only available as hardware format under OpenGL
    eTF_EAC_R11,
    eTF_EAC_RG11,
    eTF_ETC2,
    eTF_ETC2A,

    // only available as hardware format under DX9
    eTF_A8L8,
    eTF_L8,
    eTF_L8V8U8,
    eTF_B8G8R8,
    eTF_L8V8U8X8,
    eTF_B8G8R8X8,
    eTF_B8G8R8A8,

    eTF_PVRTC2,
    eTF_PVRTC4,

    eTF_ASTC_4x4,
    eTF_ASTC_5x4,
    eTF_ASTC_5x5,
    eTF_ASTC_6x5,
    eTF_ASTC_6x6,
    eTF_ASTC_8x5,
    eTF_ASTC_8x6,
    eTF_ASTC_8x8,
    eTF_ASTC_10x5,
    eTF_ASTC_10x6,
    eTF_ASTC_10x8,
    eTF_ASTC_10x10,
    eTF_ASTC_12x10,
    eTF_ASTC_12x12,

    // add R16 unsigned int format for hardware that do not support float point rendering
    eTF_R16U,
    eTF_R16G16U,
    eTF_R10G10B10A2UI,

    eTF_MaxFormat               // unused, must be always the last in the list
};

struct SDepthTexture;

//////////////////////////////////////////////////////////////////////
// Texture object interface
class ITexture
{
protected:
    virtual ~ITexture() {}
public:
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual int ReleaseForce() = 0;

    virtual const char* GetName() const = 0;
};

//=========================================================================================
