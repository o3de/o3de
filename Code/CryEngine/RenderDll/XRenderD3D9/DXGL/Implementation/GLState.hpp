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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Declares the state types and the functions to modify them
//               and apply to a device


#ifndef __GLSTATE__
#define __GLSTATE__

#include "GLCommon.hpp"

namespace NCryOpenGL
{
    class CDevice;
    class CContext;

    struct SBlendFunction
    {
        GLenum m_eSrc;
        GLenum m_eDst;
    };

    struct SChannelBlendState
    {
        GLenum m_eEquation;
        SBlendFunction m_kFunction;
    };

    struct SColorMask
    {
        GLboolean m_abRGBA[4];
    };

    struct STargetBlendState
    {
        bool m_bEnable;
        bool m_bSeparateAlpha;
        SChannelBlendState m_kRGB;
        SChannelBlendState m_kAlpha;
        SColorMask m_kWriteMask;
    };

    struct SBlendState
    {
        STargetBlendState m_kTargets[DXGL_NUM_SUPPORTED_BLEND_STATES];
        bool m_bAlphaToCoverageEnable;
        bool m_bIndependentBlendEnable;
    };

    struct SDepthStencilState
    {
        bool m_bDepthTestingEnabled;
        GLenum m_eDepthTestFunc;
        GLboolean m_bDepthWriteMask;
        bool m_bStencilTestingEnabled;
        struct SFace
        {
            GLenum m_eFunction;
            GLenum m_eStencilFailOperation;
            GLenum m_eDepthFailOperation;
            GLenum m_eDepthPassOperation;
            GLuint m_uStencilWriteMask;
            GLuint m_uStencilReadMask;
        } m_kStencilFrontFaces, m_kStencilBackFaces;
    };

    struct SRasterizerState
    {
        bool m_bCullingEnabled;
        GLenum m_eCullFaceMode;
        GLenum m_eFrontFaceMode;
        bool m_bPolygonOffsetEnabled;
        GLfloat m_fPolygonOffsetUnits;
        GLfloat m_fPolygonOffsetFactor;
        bool m_bScissorEnabled;
        bool m_bDepthClipEnabled;
#if !DXGLES
        GLenum m_ePolygonMode;
        bool m_bLineSmoothEnabled;
        bool m_bMultisampleEnabled;
#endif //!DXGLES
    };

    struct SSamplerState
    {
        // OpenGL doesn't allow a sampler object with mipmap filtering to be used with
        // textures without mipmaps. For this reason two different sampler objects are needed.
        GLuint m_uSamplerObjectMip;
        GLuint m_uSamplerObjectNoMip;
    };

    bool InitializeBlendState(const D3D11_BLEND_DESC& kDesc, SBlendState& kState, CContext* pContext);
    bool InitializeDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& kDesc, SDepthStencilState& kState, CContext* pContext);
    bool InitializeRasterizerState(const D3D11_RASTERIZER_DESC& kDesc, SRasterizerState& kState, CContext* pContext);
    bool InitializeSamplerState(const D3D11_SAMPLER_DESC& kDesc, SSamplerState& kState, CContext* pContext);
    void ResetSamplerState(SSamplerState& kSamplerState);
    bool GetTexFilterParams(GLenum& eMinFilter, GLenum& eMagFilter, bool& bAnisotropic, bool& bComparison, D3D11_FILTER eD3DFilter, bool bMip);

    inline bool operator==(SBlendFunction kLeft, SBlendFunction kRight)
    {
        return
            kLeft.m_eSrc == kRight.m_eSrc &&
            kLeft.m_eDst == kRight.m_eDst;
    }

    inline bool operator==(SChannelBlendState kLeft, SChannelBlendState kRight)
    {
        return
            kLeft.m_eEquation == kRight.m_eEquation &&
            kLeft.m_kFunction == kRight.m_kFunction;
    }

    inline bool operator==(SColorMask kLeft, SColorMask kRight)
    {
        return 0 == memcmp(kLeft.m_abRGBA, kRight.m_abRGBA, sizeof(kRight.m_abRGBA));
    }

    inline bool operator==(const STargetBlendState& kLeft, const STargetBlendState& kRight)
    {
        return
            kLeft.m_bEnable                         == kRight.m_bEnable                     &&
            kLeft.m_bSeparateAlpha          == kRight.m_bSeparateAlpha      &&
            kLeft.m_kRGB                                == kRight.m_kRGB                            &&
            kLeft.m_kAlpha                          == kRight.m_kAlpha                      &&
            kLeft.m_kWriteMask                  == kRight.m_kWriteMask;
    }

    inline bool operator!=(SBlendFunction kLeft, SBlendFunction kRight)
    {
        return !(kLeft == kRight);
    }

    inline bool operator!=(SChannelBlendState kLeft, SChannelBlendState kRight)
    {
        return !(kLeft == kRight);
    }

    inline bool operator!=(SColorMask kLeft, SColorMask kRight)
    {
        return !(kLeft == kRight);
    }

    inline bool operator!=(const STargetBlendState& kLeft, const STargetBlendState& kRight)
    {
        return !(kLeft == kRight);
    }
}


#endif //__GLSTATE__

