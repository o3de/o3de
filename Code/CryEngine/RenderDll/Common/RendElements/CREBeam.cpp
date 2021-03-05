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
#include "RendElement.h"
#include "I3DEngine.h"

void CREBeam::mfPrepare(bool bCheckOverflow)
{
    CRenderer* rd = gRenDev;

    if (bCheckOverflow)
    {
        rd->FX_CheckOverflow(0, 0, this);
    }

    CRenderObject* obj = rd->m_RP.m_pCurObject;

    if (CRenderer::CV_r_beams == 0)
    {
        rd->m_RP.m_pRE = NULL;
        rd->m_RP.m_RendNumIndices = 0;
        rd->m_RP.m_RendNumVerts = 0;
    }
    else
    {
        const int nThreadID = rd->m_RP.m_nProcessThreadID;
        SRenderObjData* pOD = obj->GetObjData();
        if (pOD)
        {
            SRenderLight* pLight = rd->EF_GetDeferredLightByID(pOD->m_nLightID);
            if (pLight && pLight->m_Flags & DLF_PROJECT)
            {
                rd->m_RP.m_pRE = this;
                rd->m_RP.m_RendNumIndices = 0;
                rd->m_RP.m_RendNumVerts = 0;
            }
            else
            {
                rd->m_RP.m_pRE = NULL;
                rd->m_RP.m_RendNumIndices = 0;
                rd->m_RP.m_RendNumVerts = 0;
            }
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Render object data is null. This may affect lighting.");
        }
    }
}

bool CREBeam::mfCompile([[maybe_unused]] CParserBin& Parser, [[maybe_unused]] SParserFrame& Frame)
{
    return true;
}


void CREBeam::SetupGeometry(SVF_P3F_C4B_T2F* pVertices, uint16* pIndices, float fAngleCoeff, float fNear, float fFar)
{
    const int nNumSides = BEAM_RE_CONE_SIDES;
    Vec2 rotations[nNumSides];
    float fIncrement = 1.0f / (float)nNumSides;
    float fAngle = 0.0f;
    for (uint32 i = 0; i < nNumSides; i++)
    {
        sincos_tpl(fAngle, &rotations[i].x, &rotations[i].y);
        fAngle += fIncrement * gf_PI2;
    }

    float fScaleNear = fNear * fAngleCoeff;
    float fScaleFar = fFar * fAngleCoeff;

    UCol cBlack, cWhite;
    cBlack.dcolor = 0;
    cWhite.dcolor = 0xFFFFFFFF;

    for (uint32 i = 0; i < nNumSides; i++) //Near Verts
    {
        pVertices[ i ].xyz      = Vec3(fNear,  rotations[i].x * fScaleNear, rotations[i].y * fScaleNear);
        pVertices[ i ].color = cWhite;
        pVertices[ i ].st       = Vec2(rotations[i].x, rotations[i].y);
    }

    for (uint32 i = 0; i < (nNumSides); i++) // Far verts
    {
        pVertices[ i + nNumSides].xyz       = Vec3(fFar,   rotations[i].x * fScaleFar, rotations[i].y * fScaleFar);
        pVertices[ i + nNumSides].color = cWhite;
        pVertices[ i + nNumSides].st        = Vec2(rotations[i].x, rotations[i].y);
    }

    uint32 nNearCapVert = nNumSides * 2;
    uint32 nFarCapVert = nNumSides * 2 + 1;

    //near cap vert
    pVertices[ nNearCapVert ].xyz = Vec3(fNear, 0.0f, 0.0f);
    pVertices[ nNearCapVert ].color = cBlack;
    pVertices[ nNearCapVert ].st = Vec2(0, 0);

    //far cap vert
    pVertices[ nFarCapVert  ].xyz = Vec3(fFar, 0.0f, 0.0f);
    pVertices[ nFarCapVert  ].color = cWhite;
    pVertices[ nFarCapVert  ].st = Vec2(0, 0);

    for (uint32 i = 0; i < nNumSides; i++)
    {
        uint32 idx = i * 6;
        pIndices[idx]    = (i) % (nNumSides);
        pIndices[idx + 1] = (i) % (nNumSides)       + nNumSides;
        pIndices[idx + 2] = (i + 1) % (nNumSides) + nNumSides;

        pIndices[idx + 3] = (i + 1) % (nNumSides) + nNumSides;
        pIndices[idx + 4] = (i + 1) % (nNumSides);
        pIndices[idx + 5] = (i) % (nNumSides);
    }

    for (uint32 i = 0; i < nNumSides; i++) // cap plane near
    {
        uint32 idx = ((nNumSides) * 6) + (i * 3);
        pIndices[idx]       = nNearCapVert;
        pIndices[idx + 1] = (i) % (nNumSides);
        pIndices[idx + 2] = (i + 1) % (nNumSides);
    }

    for (uint32 i = 0; i < nNumSides; i++) // cap plane far
    {
        uint32 idx = ((nNumSides) * 9) + (i * 3);
        pIndices[idx]       = nFarCapVert;
        pIndices[idx + 1] = (i + 1) % (nNumSides) + nNumSides;
        pIndices[idx + 2] = (i) % (nNumSides)       + nNumSides;
    }
}
