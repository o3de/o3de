// ----------------------------------------------------------------------------
// Layouts describe resources for each type of shader in TressFX.
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

#pragma once

#include "EngineInterface.h"
#include "TressFXConstantBuffers.h"

#define TRESSFX_GET_16BYTE_INDEX(s, m) (offsetof(s, m) / 16)

struct TressFXLayouts
{
    std::unique_ptr<EI_BindLayout> pTressFXParamLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pRenderPosTanLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pSimPosTanLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pGenerateSDFLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pSimLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pApplySDFLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pBoneSkinningLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pSDFMarchingCubesLayout = nullptr;
    
    std::unique_ptr<EI_BindLayout> pShortcutDepthsAlphaLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pShortcutDepthReadLayout = nullptr;
	std::unique_ptr<EI_BindLayout> pShortcutShadeParamLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pShortCutColorReadLayout = nullptr;

    std::unique_ptr<EI_BindLayout> pPPLLFillLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pPPLLResolveLayout = nullptr;
	std::unique_ptr<EI_BindLayout> pPPLLShadeParamLayout = nullptr;

    std::unique_ptr<EI_BindLayout> pViewLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pShadowViewLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pLightLayout = nullptr;
    std::unique_ptr<EI_BindLayout> pSamplerLayout = nullptr;
};

// by default, we just get layouts from a global.
// Layouts should be reusable for all instances.
// These should be allocated by the user. e.g g_TressFXSlots = new
// TressFXSlots::SlotArraysForAllLayouts;
extern TressFXLayouts*                        g_TressFXLayouts;

void InitializeAllLayouts(EI_Device* pDevice);
void DestroyAllLayouts(EI_Device* pDevice);

inline EI_BindLayout* GetSimPosTanLayout() { return g_TressFXLayouts->pSimPosTanLayout.get(); }
inline EI_BindLayout* GetRenderPosTanLayout() { return g_TressFXLayouts->pRenderPosTanLayout.get(); }
inline EI_BindLayout* GetTressFXParamLayout() { return g_TressFXLayouts->pTressFXParamLayout.get(); }
inline EI_BindLayout* GetGenerateSDFLayout() { return g_TressFXLayouts->pGenerateSDFLayout.get(); }
inline EI_BindLayout* GetApplySDFLayout() { return g_TressFXLayouts->pApplySDFLayout.get(); }
inline EI_BindLayout* GetSimLayout() { return g_TressFXLayouts->pSimLayout.get(); }
inline EI_BindLayout* GetBoneSkinningMeshLayout() { return g_TressFXLayouts->pBoneSkinningLayout.get(); }
inline EI_BindLayout* GetSDFMarchingCubesLayout() { return g_TressFXLayouts->pSDFMarchingCubesLayout.get(); }

// Shortcut layouts
inline EI_BindLayout* GetShortCutDepthsAlphaLayout() { return g_TressFXLayouts->pShortcutDepthsAlphaLayout.get(); }
inline EI_BindLayout* GetShortCutDepthReadLayout() { return g_TressFXLayouts->pShortcutDepthReadLayout.get(); }
inline EI_BindLayout* GetShortCutShadeParamLayout() { return g_TressFXLayouts->pShortcutShadeParamLayout.get(); }
inline EI_BindLayout* GetShortCutColorReadLayout() { return g_TressFXLayouts->pShortCutColorReadLayout.get(); }

// PPLL layouts
inline EI_BindLayout* GetPPLLFillLayout() { return g_TressFXLayouts->pPPLLFillLayout.get(); }
inline EI_BindLayout* GetPPLLResolveLayout() { return g_TressFXLayouts->pPPLLResolveLayout.get(); }
inline EI_BindLayout* GetPPLLShadeParamLayout() { return g_TressFXLayouts->pPPLLShadeParamLayout.get(); }

inline EI_BindLayout* GetViewLayout() { return g_TressFXLayouts->pViewLayout.get(); }
inline EI_BindLayout* GetShadowViewLayout() { return g_TressFXLayouts->pShadowViewLayout.get(); }
inline EI_BindLayout* GetLightLayout() { return g_TressFXLayouts->pLightLayout.get(); }
inline EI_BindLayout* GetSamplerLayout() { return g_TressFXLayouts->pSamplerLayout.get(); }