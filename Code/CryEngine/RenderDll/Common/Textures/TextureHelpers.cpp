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

#include "RenderDll_precompiled.h"
#include "TextureHelpers.h"
#include "TextureManager.h"
#include "StringUtils.h"

/* -----------------------------------------------------------------------
  * These functions are used in Cry3DEngine, CrySystem, CryRenderD3D11,
  * Editor, ResourceCompilerMaterial and more
  */

//////////////////////////////////////////////////////////////////////////
namespace  TextureHelpers
{
    // [Shader System TO DO] - to support data driven the following must support texture handling differently
    bool VerifyTexSuffix(EEfResTextures texSlot, const char* texPath)
    {
        MaterialTextureSemantic&    texSemantic = CTextureManager::Instance()->GetTextureSemantic(texSlot);
        return texSemantic.suffix && (strlen(texPath) > strlen(texSemantic.suffix) && (CryStringUtils::stristr(texPath, texSemantic.suffix)));
    }

    bool VerifyTexSuffix(EEfResTextures texSlot, const string& texPath)
    {
        MaterialTextureSemantic&    texSemantic = CTextureManager::Instance()->GetTextureSemantic(texSlot);
        return texSemantic.suffix && (texPath.size() > strlen(texSemantic.suffix) && (CryStringUtils::stristr(texPath, texSemantic.suffix)));
    }

    const char* LookupTexSuffix(EEfResTextures texSlot)
    {
        MaterialTextureSemantic&    texSemantic = CTextureManager::Instance()->GetTextureSemantic(texSlot);
        return texSemantic.suffix;
    }

    int8 LookupTexPriority(EEfResTextures texSlot)
    {
        MaterialTextureSemantic&    texSemantic = CTextureManager::Instance()->GetTextureSemantic(texSlot);
        return texSemantic.priority;
    }

    CTexture* LookupTexDefault(EEfResTextures texSlot)
    {
        MaterialTextureSemantic&    texSemantic = CTextureManager::Instance()->GetTextureSemantic(texSlot);
        return texSemantic.def;
    }

    // [Shader System] - To do: replace as part of data driven texture slots 
    CTexture* LookupTexNeutral(EEfResTextures texSlot)
    {
        MaterialTextureSemantic&    texSemantic = CTextureManager::Instance()->GetTextureSemantic(texSlot);
        return texSemantic.neutral;
    }
}
