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

#ifndef __TextureHelpers_h__
#define __TextureHelpers_h__
#pragma once

#include "Texture.h"

namespace TextureHelpers
{
    //////////////////////////////////////////////////////////////////////////
    bool VerifyTexSuffix(EEfResTextures texSlot, const char* texPath);
    bool VerifyTexSuffix(EEfResTextures texSlot, const string& texPath);
    const char* LookupTexSuffix(EEfResTextures texSlot);
    int8 LookupTexPriority(EEfResTextures texSlot);
    CTexture* LookupTexDefault(EEfResTextures texSlot);
    CTexture* LookupTexNeutral(EEfResTextures texSlot);
}

#endif

