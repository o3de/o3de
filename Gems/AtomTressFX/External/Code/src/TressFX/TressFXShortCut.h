
// ----------------------------------------------------------------------------
// Interface for the shortcut method.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef AMD_TRESSFXSHORTCUT_H
#define AMD_TRESSFXSHORTCUT_H

#include "EngineInterface.h"
#include "TressFXConstantBuffers.h"
#include "TressFXSettings.h"

#include <vector>

#define TRESSFX_SHORTCUT_DEPTH_NODE_SIZE 4
#define TRESSFX_SHORTCUT_K 3
#define TRESSFX_SHORTCUT_INITIAL_DEPTH 0x3f800000

class HairStrands;

class TressFXShortCut
{
public:
    TressFXShortCut();
    ~TressFXShortCut();

    void Initialize(int width, int height);
    void Draw(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_BindSet* viewBindSet, EI_BindSet* lightBindSet);
    void UpdateShadeParameters(std::vector<const TressFXRenderingSettings*>& renderSettings);

private:
    // node size should include room for UINT next pointer.
    bool Create(EI_Device* pDevice, int width, int height);
    void Clear(EI_CommandContext& context);

    void DrawHairStrands(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_PSO* pPSO, EI_BindSet** extraBindSets, uint32_t numExtraBindSets);

    // Begin/End for various stages of hair application/rendering
    void BeginDepthsAlpha(EI_CommandContext& context);
    void EndDepthsAlpha(EI_CommandContext& context);

    void BeginDepthResolve(EI_CommandContext& context);
    void EndDepthResolve(EI_CommandContext& context);

    void BeginHairColor(EI_CommandContext& context);
    void EndHairColor(EI_CommandContext& context);

    void BeginColorResolve(EI_CommandContext& context);
    void EndColorResolve(EI_CommandContext& context);

    // Bind set creation functions
    void CreateDepthsAlphaBindSet(EI_Device* pDevice);
    void CreateDepthReadBindSet(EI_Device* pDevice);
    void CreateColorReadBindSet(EI_Device* pDevice);

    // RenderPass set creation functions
    void CreateDepthsAlphaRenderTargetSet(EI_Device* pDevice);
    void CreateDepthResolveRenderTargetSet(EI_Device* pDevice);
    void CreateHairColorRenderTargetSet(EI_Device* pDevice);
    void CreateColorResolveRenderTargetSet(EI_Device* pDevice);

    int m_nScreenWidth;
    int m_nScreenHeight;

    bool m_firstRun = true;

    std::unique_ptr<EI_Resource> m_pDepths;
    std::unique_ptr<EI_Resource> m_pInvAlpha;
    std::unique_ptr<EI_Resource> m_pColors;

    std::unique_ptr<EI_BindSet>	m_pShortCutDepthsAlphaBindSet = nullptr;
    std::unique_ptr<EI_BindSet> m_pShortCutDepthReadBindSet = nullptr;
    std::unique_ptr<EI_BindSet> m_pShortCutColorReadBindSet = nullptr;

    std::unique_ptr<EI_RenderTargetSet>	  m_pShortCutDepthsAlphaRenderTargetSet = nullptr;
    std::unique_ptr<EI_RenderTargetSet>   m_pShortCutDepthResolveRenderTargetSet = nullptr;
    std::unique_ptr<EI_RenderTargetSet>   m_pShortCutHairColorRenderTargetSet = nullptr;
    std::unique_ptr<EI_RenderTargetSet>   m_pColorResolveRenderTargetSet = nullptr;

    std::unique_ptr<EI_PSO> m_pDepthsAlphaPSO = nullptr;
    std::unique_ptr<EI_PSO> m_pDepthResolvePSO = nullptr;
    std::unique_ptr<EI_PSO> m_pHairColorPSO = nullptr;
    std::unique_ptr<EI_PSO> m_pHairResolvePSO = nullptr;

    TressFXUniformBuffer<TressFXShadeParams> m_ShadeParamsConstantBuffer;
    std::unique_ptr<EI_BindSet> m_ShadeParamsBindSet = nullptr;
};

#endif