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

// Description : Post processing common utilities


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_POSTPROCESS_POSTPROCESSUTILS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_POSTPROCESS_POSTPROCESSUTILS_H
#pragma once


struct SDepthTexture;
class CShader;

struct SPostEffectsUtils
{
    enum EDepthDownsample
    {
        eDepthDownsample_None = 0,

        eDepthDownsample_Min,
        eDepthDownsample_Max,
    };

    // Create all resources
    bool Create();

    // Release all used resources
    void Release();

    // Create a render target
    static bool CreateRenderTarget(const char* szTexName, CTexture*& pTex, int iWidth, int iHeight, const ColorF& cClear, bool bUseAlpha, bool bMipMaps = 0, ETEX_Format pTexFormat = eTF_R8G8B8A8, int nCustomID = -1, int nFlags = 0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Utilities to void some code duplication
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Begins render pass utility - for post process stuff only pass 0 assumed to be used
    static bool ShBeginPass(CShader* pShader, const CCryNameTSCRC& TechName, uint32 nFlags = 0);

    // Ends render pass utility
    static void ShEndPass();

    // Set vertex shader constant utility
    static void ShSetParamVS(const CCryNameR& pParamName, const Vec4& pParam);

    // Set pixel shader constant utility
    static void ShSetParamPS(const CCryNameR& pParamName, const Vec4& pParam);

    static void GetFullScreenTri(SVF_P3F_C4B_T2F pResult[3], int nTexWidth, int nTexHeight, float z = 0, const RECT * pSrcRegion = NULL);
    static void GetFullScreenTriWPOS(SVF_P3F_T2F_T3F pResult[3], int nTexWidth, int nTexHeight, float z = 0, const RECT * pSrcRegion = NULL);

    // Draws fullscreen aligned triangle
    static void DrawFullScreenTri(int nTexWidth, int nTexHeight, float z = 0, const RECT* pSrcRegion = NULL);
    static void DrawFullScreenTriWPOS(int nTexWidth, int nTexHeight, float z = 0, const RECT* pSrcRegion = NULL);

    // Draws static quad. Uv/size offsets handled via vertex shader.
    virtual void DrawQuadFS(CShader* pShader, bool bOutputCamVec, int nWidth, int nHeight, float x0 = 0, float y0 = 0, float x1 = 1, float y1 = 1, float z = 0) = 0;

    // Deprecated - use DrawQuadFS. Draws screen aligned quad
    static void DrawScreenQuad(int nTexWidth, int nTexHeight, float x0 = 0, float y0 = 0, float x1 = 1, float y1 = 1);

    //  Deprecated: Only used in GammaCorrection technique - Draws a generic, non-screen-aligned quad
    static void DrawQuad(int nTexWidth, int nTexHeight,
        const Vec2& vxA, const Vec2& vxB, const Vec2& vxC, const Vec2& vxD,
        const Vec2& uvA = Vec2(0, 0), const Vec2& uvB = Vec2(0, 1), const Vec2& uvC = Vec2(1, 1), const Vec2& uvD = Vec2(1, 0));

    // Sets a texture
    static void SetTexture(CTexture* pTex, int nStage, int nFilter = FILTER_LINEAR, int nClamp = 1, bool bSRGBLookup = false, DWORD dwBorderColor = 0);

    // Copy a texture into other texture
    virtual void StretchRect(CTexture* pSrc, CTexture*& pDst, bool bClearAlpha = false, bool bDecodeSrcRGBK = false, bool bEncodeDstRGBK = false, bool bBigDownsample = false, EDepthDownsample depthDownsampleMode = eDepthDownsample_None, bool bBindMultisampled = false, const RECT* srcRegion = NULL) = 0;

    // Copy screen into texture
    virtual void CopyScreenToTexture(CTexture*& pDst, const RECT* pSrcRect) = 0;

    // Apply Gaussian blur a texture
    virtual void TexBlurGaussian(CTexture* pTex, int nAmount = 1, float fScale = 1.0f, float fDistribution = 5.0f, bool bAlphaOnly = false, CTexture* pMask = 0, bool bSRGB = false, CTexture* pBlurTmp = 0) = 0;

    // Clear active render target region
    static void ClearScreen(float r, float g, float b, float a);

    static void UpdateFrustumCorners();
    static void UpdateOverscanBorderAspectRatio();

    // Special full screen pass utility functions used by GMEM path /////
    static void PrepareGmemDeferredDecals();
    static void ClearGmemGBuffer();
    /////////////////////////////////////////////////////////////////////

    // Log utility
    static void Log(const char* pszMsg)
    {
        if (gRenDev->m_logFileHandle != AZ::IO::InvalidHandle && pszMsg)
        {
            gRenDev->Logv(SRendItem::m_RecurseLevel[gRenDev->m_pRT->GetThreadList()], pszMsg);
        }
    }

    // Get current color matrix set up by global color parameters
    Matrix44& GetColorMatrix();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Math utils
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    // Linear interpolation
    static float InterpolateLinear(float p1, float p2, float t)
    {
        return p1 + (p2 - p1) * t;
    };

    // Cubic interpolation
    static float InterpolateCubic(float p1, float p2, float p3, float p4, float t)
    {
        float t2 = t * t;
        return (((-p1 * 2.0f) + (p2 * 5.0f) - (p3 * 4.0f) + p4) / 6.0f) * t2 * t + (p1 + p3 - (2.0f * p2)) * t2 + (((-4.0f * p1) + p2 + (p3 * 4.0f) - p4) / 6.0f) * t + p2;
    };

    // Sine interpolation
    static float InterpolateSine(float p1, float p2, float p3, float p4, float t)
    {
        return p2 + (t * (p3 - p2)) + (sinf(t * PI) * ((p2 + p2) - p1 - p3 + (t * (p1 - (p2 + p2 + p2) + (p3 + p3 + p3) - p4))) / 8.0f);
    };

    // Return normalized random number
    static float randf()
    {
        return cry_random(0.0f, 1.0f);
    }

    // Return signed normalized random number
    static float srandf()
    {
        return cry_random(-1.0f, 1.0f);
    }

    // Returns a quasi-random sequence of values; for 2d data (2, 3) is the recommended base
    static float HaltonSequence(int index, int primeBase)
    {
        float invBase = 1.0f / (float)primeBase;
        float f = invBase;
        float result = 0;

        for (int i = index; i > 0; i /= primeBase, f *= invBase)
        {
            result += f * (float)(i % primeBase);
        }

        return result;
    }

    // Returns closest power of 2 size
    static int GetClosestPow2Size(int size)
    {
        float fPower = floorf(logf((float)size) / logf(2.0f));
        int nResize = int(powf(2.0f, fPower));

        // Clamp
        if (nResize >= 512)
        {
            nResize = 512;
        }

        return nResize;
    }

    static void GetViewMatrix(Matrix44A& viewMatrix, bool bCameraSpace = false)
    {
        viewMatrix = gRenDev->m_ViewMatrix;
        if (bCameraSpace)
        {
            viewMatrix.m30 = 0.0f;
            viewMatrix.m31 = 0.0f;
            viewMatrix.m32 = 0.0f;
        }
    }

    static void GetTextureRect(CTexture* pTexture, RECT* pRect)
    {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = pTexture->GetWidth();
        pRect->bottom = pTexture->GetHeight();
    }

    static float GaussianDistribution1D(float x, float rho)
    {
        float g = 1.0f / (rho * sqrtf(2.0f * PI));
        g *= expf(-(x * x) / (2.0f * rho * rho));
        return g;
    }

    static float GaussianDistribution2D(float x, float y, float rho)
    {
        float g = 1.0f / (2.0f * PI * rho * rho);
        g *= expf(-(x * x + y * y) / (2 * rho * rho));
        return g;
    }

    static CTexture* GetTemporalCurrentTarget()
    {
        return CTexture::s_ptexPrevBackBuffer[SPostEffectsUtils::m_iFrameCounter % 2][gRenDev->m_CurRenderEye];
    }

    static CTexture* GetTemporalHistoryTarget()
    {
        return CTexture::s_ptexPrevBackBuffer[(SPostEffectsUtils::m_iFrameCounter + 1) % 2][gRenDev->m_CurRenderEye];
    }

    static CTexture* GetCoCCurrentTarget()
    {
        return CTexture::s_ptexSceneCoCHistory[SPostEffectsUtils::m_iFrameCounter % 2];
    }

    static CTexture* GetCoCHistoryTarget()
    {
        return CTexture::s_ptexSceneCoCHistory[(SPostEffectsUtils::m_iFrameCounter + 1) % 2];
    }

    static CTexture* AcquireFinalCompositeTarget(bool bNeedHDRTarget)
    {
        if (bNeedHDRTarget)
        {
            m_UpscaleTarget = GetTemporalHistoryTarget();
        }
        else
        {
            m_UpscaleTarget = CTexture::s_ptexSceneDiffuse;
        }
        return m_UpscaleTarget;
    }

    static CTexture* GetFinalCompositeTarget()
    {
        return m_UpscaleTarget;
    }

    static CTexture* GetVelocityObjectRT()
    {
        return CTexture::s_ptexVelocityObjects[gRenDev->m_CurRenderEye];
    }

    static float GetOverscanBorderAspectRatio()
    {
        return m_fOverscanBorderAspectRatio;
    }

public:

    static SDepthTexture* m_pCurDepthSurface;
    static RECT m_pScreenRect;
    static ITimer* m_pTimer;
    static int m_iFrameCounter;
    static int m_nColorMatrixFrameID;

    static CShader* m_pCurrShader;

    Matrix44 m_pView;
    Matrix44 m_pProj;
    Matrix44 m_pViewProj;
    Matrix44 m_pColorMat;
    static Matrix44 m_pScaleBias;
    static float m_fWaterLevel;

    // frustrum corners
    static Vec3 m_vRT, m_vLT, m_vLB, m_vRB;
    static int m_nFrustrumFrameID;

protected:

    SPostEffectsUtils()
    {
        m_pView.SetIdentity();
        m_pProj.SetIdentity();
        m_pViewProj.SetIdentity();
        m_pColorMat.SetIdentity();

        m_pCurDepthSurface = NULL;
        m_pScreenRect.left = m_pScreenRect.top = 0;
        m_pScreenRect.bottom = m_pScreenRect.right = 0;
        m_pTimer = NULL;
        m_iFrameCounter = 0;
        m_nColorMatrixFrameID = -1;

        m_pCurrShader = NULL;
        m_fWaterLevel = 0.0;
        m_vRT = m_vLT = m_vLB = m_vRB = Vec3(ZERO);
        m_nFrustrumFrameID = -1;
    }

    virtual ~SPostEffectsUtils()
    {
    }

    static float m_fOverscanBorderAspectRatio;

private:
    static CTexture* m_UpscaleTarget;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_POSTPROCESS_POSTPROCESSUTILS_H
