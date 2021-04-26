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
#include "../Common/RendElements/CRELensOptics.h"


CRELensOptics::CRELensOptics(void)
{
    mfSetType(eDATA_LensOptics);
    mfUpdateFlags(FCEF_TRANSFORM);
}
CRELensOptics::~CRELensOptics(void) {}

bool CRELensOptics::mfCompile([[maybe_unused]] CParserBin& Parser, [[maybe_unused]] SParserFrame& Frame){ return true; }

void CRELensOptics::mfPrepare([[maybe_unused]] bool bCheckOverflow) {}

bool CRELensOptics::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm) { return true; }
