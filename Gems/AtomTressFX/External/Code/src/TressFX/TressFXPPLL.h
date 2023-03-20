// ----------------------------------------------------------------------------
// Interface for TressFX OIT using per-pixel linked lists.
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

#ifndef AMD_TRESSFXPPLL_H
#define AMD_TRESSFXPPLL_H

#include "EngineInterface.h"
#include "TressFXSettings.h"

#include <vector>

// By default, TressFX node sizes are 16 bytes.  4 bytes for each of:  tangentcoverage, depth,
// baseColor, next pointer
#define TRESSFX_DEFAULT_NODE_SIZE 16

// The special value meaning "end of list" in the PPLL.
// So the UAV of head pointers gets cleared to this.
#define TRESSFX_PPLL_NULL_PTR 0xffffffff

class HairStrands;

class TressFXPPLL
{
public:
    TressFXPPLL();

    void Initialize(int width, int height, int nNodes, int nodeSize);
    void Draw(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_BindSet* viewBindSet, EI_BindSet* lightBindSet);

    void UpdateShadeParameters(std::vector<const TressFXRenderingSettings*>& renderSettings);

private:
    // Node size should include room for UINT next pointer.
    void Clear(EI_CommandContext& context);
    bool Create(EI_Device* pDevice, int width, int height, int nNodes, int nodeSize = TRESSFX_DEFAULT_NODE_SIZE);

    void DrawHairStrands(EI_CommandContext& commandContext, int numHairStrands, HairStrands** hairStrands, EI_PSO* pPSO, EI_BindSet** extraBindSets, uint32_t numExtraBindSets);

    // Begin/End for various stages of hair application/rendering
    void BeginFill(EI_CommandContext& context);
    void EndFill(EI_CommandContext& context);

    void BeginResolve(EI_CommandContext& context);
    void EndResolve(EI_CommandContext& context);

    // Bind set creation functions
    void CreateFillBindSet(EI_Device* pDevice);
    void CreateResolveBindSet(EI_Device* pDevice);

    // RenderPass set creation functions
    void CreatePPLLRenderTargetSet(EI_Device* pDevice);

    int m_nScreenWidth;
    int m_nScreenHeight;
    int m_nNodes;
    int m_nNodeSize;

    bool m_firstRun = true;

    std::unique_ptr<EI_Resource> m_PPLLHeads;
    std::unique_ptr<EI_Resource> m_PPLLNodes;
    std::unique_ptr<EI_Resource> m_PPLLCounter;

    std::unique_ptr<EI_BindSet> m_pPPLLFillBindSet;
    std::unique_ptr<EI_BindSet> m_pPPLLResolveBindSet;

    std::unique_ptr<EI_RenderTargetSet> m_PPLLRenderTargetSet = nullptr;

    std::unique_ptr<EI_PSO> m_PPLLFillPSO = nullptr;
    std::unique_ptr<EI_PSO> m_PPLLResolvePSO = nullptr;

    TressFXUniformBuffer<TressFXShadeParams> m_ShadeParamsConstantBuffer;
    std::unique_ptr<EI_BindSet> m_ShadeParamsBindSet = nullptr;
};

#endif