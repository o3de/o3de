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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DCOLORGRADINGCONTROLLER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DCOLORGRADINGCONTROLLER_H
#pragma once

#include <RenderDll/Common/IColorGradingControllerInt.h>


class CD3D9Renderer;

struct SColorGradingMergeParams
{
    Vec4 pColorMatrix[3];
    Vec4 pLevels[2];
    Vec4 pFilterColor;
    Vec4 pSelectiveColor[2];
    uint64 nFlagsShaderRT;
};

class CColorGradingControllerD3D
    : public IColorGradingControllerInt
{
public:
    // IColorGradingController interface
    virtual int LoadColorChart(const char* pChartFilePath) const;
    virtual int LoadDefaultColorChart() const;
    virtual void UnloadColorChart(int texID) const;

    virtual void SetLayers(const SColorChartLayer* pLayers, uint32 numLayers);

public:
    // IColorGradingController internal interface
    virtual void RT_SetLayers(const SColorChartLayer* pLayers, uint32 numLayers);

public:
    CColorGradingControllerD3D(CD3D9Renderer* pRenderer);
    virtual ~CColorGradingControllerD3D();

    bool Update(const SColorGradingMergeParams* pMergeParams = 0);
    CTexture* GetColorChart() const;
    void DrawDebugInfo() const;

    bool LoadStaticColorChart(const char* pChartFilePath);
    const CTexture* GetStaticColorChart() const;

    void ReleaseTextures();
    void FreeMemory();

private:
    typedef std::vector<SColorChartLayer> Layers;

private:
    bool ValidateColorChart(const CTexture* pChart) const;
    CTexture* LoadColorChartInt(const char* pChartFilePath) const;
    bool InitResources();
    void DrawLayer(float x, float y, float w, float h, CTexture* pChart, float blendAmount, const char* pLayerName) const;

private:
    Layers m_layers;
    CD3D9Renderer* m_pRenderer;
    CVertexBuffer* m_pSlicesVB;
    std::vector<SVF_P3F_C4B_T2F> m_vecSlicesData;
    CTexture* m_pChartIdentity;
    CTexture* m_pChartStatic;
    CTexture* m_pChartToUse;
    CTexture* m_pMergeLayers[2];
};


#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DCOLORGRADINGCONTROLLER_H
