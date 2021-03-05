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
#include "RenderDll_precompiled.h"
#include <Common/RenderCapabilities.h>
#include "Implementation/GLCommon.hpp"

namespace RenderCapabilities
{
    bool SupportsTextureViews()
    {
        return NCryMetal::SupportTextureViews();
    }

    bool SupportsStencilTextures()
    {
        return NCryMetal::SupportsStencilTextures();
    }

    int GetAvailableMRTbpp()
    {
        return NCryMetal::GetAvailableMRTbpp();
    }

    bool Supports128bppGmemPath()
    {
        return GetAvailableMRTbpp() >= 128;
    }

    bool Supports256bppGmemPath()
    {
        return GetAvailableMRTbpp() >= 256;
    }

    bool SupportsPLSExtension()
    {
        return false;
    }

    FrameBufferFetchMask GetFrameBufferFetchCapabilities()
    {
        FrameBufferFetchMask mask;
        mask.set(FBF_ALL_COLORS);
        mask.set(FBF_COLOR0);
        return mask;
    }
    
    bool SupportsDepthClipping()
    {
        //https://developer.apple.com/documentation/metal/mtlrendercommandencoder/1516267-setdepthclipmode?language=objc
#if defined(AZ_PLATFORM_MAC)
        //There is a bug with the drivers where setDepthClipMode: MTLDepthClipModeClamp does not work. 
        //Until that is fixed we are simulating this behavior in the vertex shader
        //return NCryMetal::s_isOsxMinVersion10_11;
        return false;
#else
        //There is a bug with the drivers where setDepthClipMode: MTLDepthClipModeClamp does not work. 
        //Until that is fixed we are simulating this behavior in the vertex shader
        //return NCryMetal::s_isIosMinVersion11_0;
        return false;
#endif
    }
    
    bool SupportsDualSourceBlending()
    {
        // Metal supports dual source blending for devices running OXS >= 10.12 or iOS >= 11.0
        // but you need to declare the "index" of the render target in the shader (half4 Source1 [[ color(0), index(1) ]])
        // Unfortunately HLSLcc is not able to generate this type of declaration because the DX Shader bytecode doesn't
        // distinguish between a normal output or one for dual source blending.
        return false;
    }    

    
    bool SupportsRenderTargets(int numRTs)
    {
#if defined(AZ_PLATFORM_MAC)
        return true;
#else
        if (numRTs <= 4)
        {
            //128bpp means it will support 4 render targets
            return GetAvailableMRTbpp() >= 128;
        }
        else if (numRTs <= 8)        
        {
            //256bpp means it will support 8 render targets
            return GetAvailableMRTbpp() >= 256;
        }

        return false;        
#endif
    }
    
    bool SupportsStructuredBuffer(EShaderStage stage)
    {
        return true;
    }

    bool SupportsIndependentBlending()
    {
        return true;
    }
}
