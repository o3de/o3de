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
#include "../../Common/RenderCapabilities.h"
#include "Implementation/GLCommon.hpp"
#include "Interfaces/CCryDXGLDevice.hpp"
#include "Implementation/GLDevice.hpp"
#include <AzCore/std/algorithm.h>

namespace RenderCapabilities
{
    NCryOpenGL::CDevice* GetGLDevice()
    {
        AZ_Assert(gcpRendD3D, "gcpRendD3D is NULL");
        CCryDXGLDevice* pDXGLDevice(CCryDXGLDevice::FromInterface(static_cast<ID3D11Device*>(&gcpRendD3D->GetDevice())));
        AZ_Assert(pDXGLDevice, "CCryDXGLDevice is NULL");
        return pDXGLDevice->GetGLDevice();
    }

    bool SupportsTextureViews()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_TextureViews);
    }

    bool SupportsStencilTextures()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_StencilTextures);
    }

    bool SupportsDepthClipping()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_DepthClipping);
    }

    bool SupportsDualSourceBlending()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_DualSourceBlending);
    }

    bool SupportsStructuredBuffer(EShaderStage stageMask)
    {
        AZStd::vector<NCryOpenGL::EShaderType> shaderStages;
        if (stageMask & EShaderStage_Vertex)
        {
            shaderStages.push_back(NCryOpenGL::eST_Vertex);
        }

        if (stageMask & EShaderStage_Pixel)
        {
            shaderStages.push_back(NCryOpenGL::eST_Fragment);
        }

        if (stageMask & EShaderStage_Geometry)
        {
#if DXGL_SUPPORT_GEOMETRY_SHADERS
            shaderStages.push_back(NCryOpenGL::eST_Geometry);
#else
            return false;
#endif
        }

        if (stageMask & EShaderStage_Compute)
        {
#if DXGL_SUPPORT_COMPUTE
            shaderStages.push_back(NCryOpenGL::eST_Compute);
#else
            return false;
#endif
        }

        if (stageMask & EShaderStage_Domain)
        {
#if DXGL_SUPPORT_TESSELLATION
            shaderStages.push_back(NCryOpenGL::eST_TessEvaluation);
#else
            return false;
#endif
        }

        if (stageMask & EShaderStage_Hull)
        {
#if DXGL_SUPPORT_TESSELLATION
            shaderStages.push_back(NCryOpenGL::eST_TessControl);
#else
            return false;
#endif
        }

        const NCryOpenGL::SResourceUnitCapabilities& capabilities = GetGLDevice()->GetAdapter()->m_kCapabilities.m_akResourceUnits[NCryOpenGL::eRUT_StorageBuffer];
        for (AZStd::vector<NCryOpenGL::EShaderType>::iterator it = shaderStages.begin(); it != shaderStages.end(); ++it)
        {
            if (capabilities.m_aiMaxPerStage[*it] == 0)
            {
                return false;
            }
        }

        return true;
    }

#if defined(OPENGL_ES)
    int GetAvailableMRTbpp()
    {
        const NCryOpenGL::SCapabilities& capabilities = GetGLDevice()->GetAdapter()->m_kCapabilities;
        if (GetFrameBufferFetchCapabilities().test(FBF_ALL_COLORS))
        {
            // Assuming 32 bits per rendertarget when using all attachments.
            GLint bitsPerRT = 32;
            GLint numRT = capabilities.m_maxRenderTargets;
            return numRT * bitsPerRT;
        }

        if (SupportsPLSExtension())
        {
            GLint bitsPerByte = 8;
            GLint plsSize = capabilities.m_plsSizeInBytes;
            // We only support PLS 128 for the moment.
            return AZStd::min(plsSize * bitsPerByte, 128);
        }

        return 0;
    }

    bool Supports128bppGmemPath()
    {
        // We don't support GMEM on platforms without floating point rendertarget support 
        return SupportsHalfFloatRendering() && 
            (GetFrameBufferFetchCapabilities().test(FBF_ALL_COLORS) || SupportsPLSExtension()) &&
            GetAvailableMRTbpp() >= 128;
    }

    bool Supports256bppGmemPath()
    {
        // We don't support GMEM on platforms without floating point rendertarget support 
        return SupportsHalfFloatRendering() && 
            (GetFrameBufferFetchCapabilities().test(FBF_ALL_COLORS) || SupportsPLSExtension())
            && GetAvailableMRTbpp() >= 256;
    }

    bool SupportsHalfFloatRendering()
    {
        return DXGL_GL_EXTENSION_SUPPORTED(EXT_color_buffer_half_float);
    }

    bool SupportsRenderTargets(int numRTs)
    {
        const NCryOpenGL::SCapabilities& capabilities = GetGLDevice()->GetAdapter()->m_kCapabilities;
        return capabilities.m_maxRenderTargets >= numRTs;
    }
#endif

    bool SupportsPLSExtension()
    {
        // Favor framebuffer fetch over PLS (for compatibility with Metal)
        const NCryOpenGL::SCapabilities& capabilities = GetGLDevice()->GetAdapter()->m_kCapabilities;
        return !GetFrameBufferFetchCapabilities().test(FBF_ALL_COLORS) && capabilities.m_plsSizeInBytes > 0;
    }

    FrameBufferFetchMask GetFrameBufferFetchCapabilities()
    {
        const NCryOpenGL::SCapabilities& capabilities = GetGLDevice()->GetAdapter()->m_kCapabilities;
        return capabilities.m_frameBufferFetchSupport;
    }

    bool SupportsIndependentBlending()
    {
        return GetGLDevice()->IsFeatureSupported(NCryOpenGL::eF_IndependentBlending);
    }

    uint32 GetDeviceGLVersion()
    {
        return GetGLDevice()->GetFeatureSpec().m_kVersion.ToUint();
    }
}