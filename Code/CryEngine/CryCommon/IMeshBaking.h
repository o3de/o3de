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

#ifndef CRYINCLUDE_CRYCOMMON_IMESHBAKING_H
#define CRYINCLUDE_CRYCOMMON_IMESHBAKING_H
#pragma once


struct SMeshBakingMaterialParams
{
    float rayLength;
    float rayIndent;
    bool bAlphaCutout;
    bool bIgnore;
};

struct SMeshBakingInputParams
{
    IStatObj* pCageMesh;
    IStatObj* pInputMesh;
    const SMeshBakingMaterialParams* pMaterialParams;
    ColorF defaultBackgroundColour;
    ColorF dilateMagicColour;
    int outputTextureWidth;
    int outputTextureHeight;
    int numMaterialParams;
    int nLodId;
    bool bDoDilationPass;
    bool bSmoothNormals;
    bool bSaveSpecular;
    _smart_ptr<IMaterial> pMaterial;
};

struct SMeshBakingOutput
{
    ITexture* ppOuputTexture[3];
    ITexture* ppIntermediateTexture[3];
};

#endif // CRYINCLUDE_CRYCOMMON_IMESHBAKING_H
