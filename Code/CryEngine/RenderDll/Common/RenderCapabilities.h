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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDERCAPABILITIES_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDERCAPABILITIES_H
#pragma once

#include <XRenderD3D9/DeviceManager/Enums.h>
#include <AzCore/std/containers/bitset.h>

namespace RenderCapabilities
{
    // GPU Vendor ID list
    static const unsigned int s_gpuVendorIdNVIDIA = 0x10de;
    static const unsigned int s_gpuVendorIdAMD = 0x1002;
    static const unsigned int s_gpuVendorIdIntel = 0x8086;
    static const unsigned int s_gpuVendorIdQualcomm = 0x5143;
    static const unsigned int s_gpuVendorIdSamsung = 0x1099;
    static const unsigned int s_gpuVendorIdARM = 0x13B5;

    //Note that for platforms that don't support texture views, you are still allowed to create a single view, that view must "match" the creation parameters of
    //the texture.
    bool SupportsTextureViews();

    // Test to determine if stencil textures are supported
    bool SupportsStencilTextures();

#if defined(OPENGL_ES) || defined(CRY_USE_METAL)
    // Test to determine how much MRT bpp is available. (-1 means no restriction)
    int GetAvailableMRTbpp();

    // Tests to see if GMEM 128bpp path is supported.
    bool Supports128bppGmemPath();

    // Tests to see if GMEM 256bpp path is supported.
    bool Supports256bppGmemPath();

    // Tests to see if the device can support enough renderTargets.
    bool SupportsRenderTargets(int numRTs);
#endif

#if defined(OPENGL_ES)
    bool SupportsHalfFloatRendering();
#endif

#if defined(OPENGL_ES) || defined(OPENGL)
    uint32 GetDeviceGLVersion();
#endif
    
    //Check if Depth clipping API is enabled
    bool SupportsDepthClipping();

    // Flags for Frame Buffer Fetch capabilities
    enum
    {
        FBF_ALL_COLORS  = 0,// Can fetch from any color render target that is bound. 
        FBF_COLOR0,         // Some devices only allows fetching from the first color render target.
        FBF_DEPTH,          // Can fetch the depth value from the attached buffer.
        FBF_STENCIL,        // Can fetch the stencil value from the attached buffer.
        FBF_COUNT
    };

    using FrameBufferFetchMask = AZStd::bitset<FBF_COUNT>;

    FrameBufferFetchMask GetFrameBufferFetchCapabilities();

    // Extracting this out as to not pollute rest of code base with a bunch of "if defined(OPENGL_ES)"
    bool SupportsPLSExtension();

    bool SupportsDualSourceBlending();

    bool SupportsStructuredBuffer(EShaderStage stage);

    bool SupportsIndependentBlending();
}

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDERCAPABILITIES_H
