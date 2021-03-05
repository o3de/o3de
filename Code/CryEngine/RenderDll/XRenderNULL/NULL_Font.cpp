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

// Description : NULL font functions.


#include "RenderDll_precompiled.h"
#include "NULL_Renderer.h"
#include "Common/Textures/TextureManager.h"

#include "../CryFont/FBitmap.h"

bool CNULLRenderer::FontUpdateTexture([[maybe_unused]] int nTexId, [[maybe_unused]] int X, [[maybe_unused]] int Y, [[maybe_unused]] int USize, [[maybe_unused]] int VSize, [[maybe_unused]] byte* pData)
{
    return true;
}

bool CNULLRenderer::FontUploadTexture([[maybe_unused]] class CFBitmap* pBmp, [[maybe_unused]] ETEX_Format eTF)
{
    return true;
}
void CNULLRenderer::FontReleaseTexture([[maybe_unused]] class CFBitmap* pBmp)
{
}

void CNULLRenderer::FontSetTexture([[maybe_unused]] class CFBitmap* pBmp, [[maybe_unused]] int nTexFiltMode)
{
}

void CNULLRenderer::FontSetTexture([[maybe_unused]] int nTexId, [[maybe_unused]] int nFilterMode)
{
}

int CNULLRenderer::FontCreateTexture([[maybe_unused]] int Width, [[maybe_unused]] int Height, [[maybe_unused]] byte* pData, [[maybe_unused]] ETEX_Format eTF, [[maybe_unused]] bool genMips, [[maybe_unused]] const char* textureName)
{
    return CTextureManager::Instance()->GetNoTexture()->GetTextureID();
}

void CNULLRenderer::DrawDynVB([[maybe_unused]] SVF_P3F_C4B_T2F* pBuf, [[maybe_unused]] uint16* pInds, [[maybe_unused]] int nVerts, [[maybe_unused]] int nInds, [[maybe_unused]] const PublicRenderPrimitiveType nPrimType)
{
}

void CNULLRenderer::DrawDynUiPrimitiveList([[maybe_unused]] DynUiPrimitiveList& primitives, [[maybe_unused]] int totalNumVertices, [[maybe_unused]] int totalNumIndices)
{
}