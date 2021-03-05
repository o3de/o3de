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

// Description : Implementation of the shadow maps using NULL device specific implementation
//               shadow map calculations


#include "RenderDll_precompiled.h"
#include "NULL_Renderer.h"
#include "../Common/Shadow_Renderer.h"

#include "I3DEngine.h"

IDynTexture* CNULLRenderer::MakeDynTextureFromShadowBuffer([[maybe_unused]] int nSize, [[maybe_unused]] IDynTexture* pDynTexture)
{
    return NULL;
}

bool CNULLRenderer::PrepareDepthMap([[maybe_unused]] ShadowMapFrustum* SMSource, [[maybe_unused]] int nFrustumLOD, [[maybe_unused]] bool bClearPool)
{
    return true;
}

void CNULLRenderer::DrawAllShadowsOnTheScreen()
{
}
