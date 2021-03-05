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
#include "CRECloud.h"
#include "../Textures/TextureManager.h"

#include <I3DEngine.h>

uint32  CRECloud::m_siShadeResolution = 32;
float CRECloud::m_sfAlbedo = 0.9f;
float CRECloud::m_sfExtinction = 80.0f;
float CRECloud::m_sfTransparency = exp(-m_sfExtinction);
float CRECloud::m_sfScatterFactor = m_sfAlbedo * m_sfExtinction * (1.0f / (4.0f * (float)M_PI));
float CRECloud::m_sfSortAngleErrorTolerance = 0.8f;
float CRECloud::m_sfSortSquareDistanceTolerance = 100.0f;

void CRECloud::SortParticles(const Vec3& vViewDir, const Vec3& vSortPoint, ESortDirection eDir)
{
    Vec3 partPos;
    for (uint32 i = 0; i < m_particles.size(); ++i)
    {
        partPos = m_particles[i]->GetPosition();
        partPos -= vSortPoint;
        m_particles[i]->SetSquareSortDistance(partPos * vViewDir);
    }

    switch (eDir)
    {
    case eSort_TOWARD:
        std::sort(m_particles.begin(), m_particles.end(), m_towardComparator);
        break;
    case eSort_AWAY:
        std::sort(m_particles.begin(), m_particles.end(), m_awayComparator);
        break;
    default:
        break;
    }
}

void CRECloud::GetIllumParams(ColorF& specColor, ColorF& diffColor)
{
    CShaderResources* pRes = gRenDev->m_RP.m_pShaderResources;
    if (pRes && pRes->HasLMConstants())
    {
        specColor = pRes->GetColorValue(EFTT_SPECULAR);
        diffColor = pRes->GetColorValue(EFTT_DIFFUSE);
    }
    else
    {
        ColorF col = gRenDev->m_RP.m_pSunLight->m_Color;
        float fLum = col.Luminance();
        col.NormalizeCol(diffColor);
        specColor.a = 1.0f;
        specColor = specColor * fLum / 1.5f;
        diffColor = gRenDev->m_RP.m_pCurObject->m_II.m_AmbColor / 5.0f;
    }
}

void CRECloud::ShadeCloud([[maybe_unused]] Vec3 vPos)
{
    ColorF specColor, diffColor;
    if (gRenDev->m_RP.m_pSunLight)
    {
        GetIllumParams(specColor, diffColor);
        //IlluminateCloud(gRenDev->m_RP.m_pSunLight->m_Origin, vPos, difColor, ambColor, true);
        m_CurSpecColor = specColor;
        m_CurDiffColor = diffColor;
        m_bReshadeCloud = false;
        if (gRenDev->m_RP.m_pCurObject && gRenDev->m_RP.m_pCurObject->GetRE())
        {
            CREImposter* pRE = (CREImposter*)gRenDev->m_RP.m_pCurObject->GetRE();
            pRE->m_bScreenImposter = true;
        }
    }
}

void CRECloud::UpdateWorldSpaceBounds(CRenderObject* pObj)
{
    CREImposter* pRE = (CREImposter*)pObj->GetRE();
    assert(pRE);
    if (!pRE)
    {
        return;
    }
    pRE->m_WorldSpaceBV = m_boundingBox;
    if (m_Flags & FCEF_OLD)
    {
        Matrix34 mScale = Matrix34::CreateScale(Vec3(m_fScale, m_fScale, m_fScale));
        pRE->m_WorldSpaceBV.Transform(mScale);
    }
    pRE->m_WorldSpaceBV.Transform(pObj->m_II.m_Matrix);
}

void CRECloud::mfPrepare(bool bCheckOverflow)
{
    CRenderer* rd = gRenDev;

    if (bCheckOverflow)
    {
        rd->FX_CheckOverflow(0, 0, this);
    }

    CRenderObject* pObj = rd->m_RP.m_pCurObject;
    CREImposter* pRE = (CREImposter*)pObj->GetRE();
    SRenderObjData* pOD = pObj->GetObjData();
    assert(pOD);
    if (!pOD)
    {
        return;
    }
    if (!pRE)
    {
        pRE = new CREImposter;
        pObj->m_pRE = pRE;
        pRE->m_State = GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA | GS_ALPHATEST_GREATER;
        pRE->m_AlphaRef = 0;
    }

    if (pOD->m_fTempVars[0] != pOD->m_fTempVars[1])
    {
        pOD->m_fTempVars[1] = pOD->m_fTempVars[0];
        m_bReshadeCloud = true;
    }
    if (m_Flags & FCEF_OLD)
    {
        float fCloudRadius = m_boundingBox.GetRadius();
        m_fScale = pOD->m_fTempVars[0] / fCloudRadius;
    }
    else
    {
        m_fScale = pOD->m_fTempVars[0];
    }

    ColorF specColor, diffColor;
    GetIllumParams(specColor, diffColor);
    if (specColor != m_CurSpecColor || diffColor != m_CurDiffColor)
    {
        m_bReshadeCloud = true;
    }

    UpdateWorldSpaceBounds(pObj);
    Vec3 vPos = pRE->GetPosition();

    {
        if (m_bReshadeCloud)
        {
            ShadeCloud(vPos);
        }
        UpdateImposter(pObj);
    }

    rd->m_RP.m_pCurObject = pObj;
    rd->m_RP.m_pRE = this;
    rd->m_RP.m_RendNumIndices = 0;
    rd->m_RP.m_RendNumVerts = 4;
    rd->m_RP.m_FirstVertex = 0;
}

bool CRECloud::mfLoadCloud(const string& name, float fScale, [[maybe_unused]] bool bLocal)
{
    uint32 i;
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(name.c_str(), "rb");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    uint32 iNumParticles;

    mfSetFlags(FCEF_OLD);

    gEnv->pCryPak->FRead(&iNumParticles, 1, fileHandle);
    if (iNumParticles == 0x238c)
    {
        char fTexName[128];
        char texName[256];
        gEnv->pCryPak->FRead(&iNumParticles, 1, fileHandle);
        int n = 0;
        int ch;
        do
        {
            ch = gEnv->pCryPak->Getc(fileHandle);
            fTexName[n++] = ch;
            if (n > 128)
            {
                fTexName[127] = ch;
                break;
            }
        } while (ch != 0);
        fpStripExtension(fTexName, fTexName);
        sprintf_s(texName, "Textures/Clouds/%s.dds", fTexName);
        m_pTexParticle = CTexture::ForName(texName, 0, eTF_Unknown);
        gEnv->pCryPak->FRead(&m_nNumColorGradients, 1, fileHandle);
        //m_pColorGradients = new SColorLevel [m_nNumColorGradients];
        for (i = 0; i < m_nNumColorGradients; i++)
        {
            float fLevel;
            gEnv->pCryPak->FRead(&fLevel, 1, fileHandle);
            //m_pColorGradients[i].m_fLevel /= 100.0f;
            uint32 iColor;
            gEnv->pCryPak->FRead(&iColor, 1, fileHandle);
            //m_pColorGradients[i].m_vColor = ColorF(iColor);
        }
        for (i = 0; i < iNumParticles; ++i)
        {
            Vec3 vPosition;
            short nShadingNum;
            short nGroupNum;
            short nWidthMin, nWidthMax;
            short nLengthMin, nLengthMax;
            short nRotMin, nRotMax;
            Vec2 vUV[2];
            gEnv->pCryPak->FRead(&vPosition, 1, fileHandle);
            gEnv->pCryPak->FRead(&nShadingNum, 1, fileHandle);
            gEnv->pCryPak->FRead(&nGroupNum, 1, fileHandle);
            gEnv->pCryPak->FRead(&nWidthMin, 1, fileHandle);
            gEnv->pCryPak->FRead(&nWidthMax, 1, fileHandle);
            gEnv->pCryPak->FRead(&nLengthMin, 1, fileHandle);
            gEnv->pCryPak->FRead(&nLengthMax, 1, fileHandle);
            gEnv->pCryPak->FRead(&nRotMin, 1, fileHandle);
            gEnv->pCryPak->FRead(&nRotMax, 1, fileHandle);
            gEnv->pCryPak->FRead(&vUV[0], 1, fileHandle);
            gEnv->pCryPak->FRead(&vUV[1], 1, fileHandle);

            vPosition *= 0.001f;
            float fWidth = (float)nWidthMin * 0.001f;
            float fHeight = (float)nLengthMin * 0.001f;
            float fRotMin = (float)nRotMin;
            float fRotMax = (float)nRotMax;
            Exchange(vPosition.y, vPosition.z);
            vUV[0].y = 1.0f - vUV[0].y;
            vUV[1].y = 1.0f - vUV[1].y;
            Exchange(vUV[0].x, vUV[1].x);
            /*int nX = nShadingNum & 0x3;
            int nY = nShadingNum >> 2;
            vUV[0].x = (float)nX * 0.25f;
            vUV[1].x = vUV[0].x + 0.25f;
            vUV[0].y = (float)nY * 0.25f;
            vUV[1].y = vUV[0].y + 0.25f;*/
            SCloudParticle* pParticle = new SCloudParticle(vPosition, fWidth, fHeight, fRotMin, fRotMax, vUV);

            Vec3 vMin = pParticle->GetPosition() - Vec3(fWidth, fWidth, fHeight);
            Vec3 vMax = pParticle->GetPosition() + Vec3(fWidth, fWidth, fHeight);
            m_boundingBox.AddPoint(vMin);
            m_boundingBox.AddPoint(vMax);
            m_particles.push_back(pParticle);
        }
        Vec3 vCenter = m_boundingBox.GetCenter();
        if (vCenter != Vec3(0, 0, 0))
        {
            m_boundingBox.Clear();
            for (i = 0; i < iNumParticles; i++)
            {
                SCloudParticle* pParticle = m_particles[i];
                pParticle->SetPosition(pParticle->GetPosition() - vCenter);
                float fWidth = pParticle->GetRadiusX();
                float fHeight = pParticle->GetRadiusY();
                Vec3 vMin = pParticle->GetPosition() - Vec3(fWidth, fWidth, fHeight);
                Vec3 vMax = pParticle->GetPosition() + Vec3(fWidth, fWidth, fHeight);
                m_boundingBox.AddPoint(vMin);
                m_boundingBox.AddPoint(vMax);
            }
        }
    }
    else
    {
        Vec3 vCenter = Vec3(0, 0, 0);
        gEnv->pCryPak->FRead(&vCenter[0], 1, fileHandle);
        vCenter = Vec3(0, 0, 0);

        Vec3* pParticlePositions = new Vec3[iNumParticles];
        float* pParticleRadii     = new float[iNumParticles];
        ColorF* pParticleColors    = new ColorF[iNumParticles];

        gEnv->pCryPak->FRead(pParticlePositions, iNumParticles, fileHandle);
        gEnv->pCryPak->FRead(pParticleRadii, iNumParticles, fileHandle);
        gEnv->pCryPak->FRead(pParticleColors, iNumParticles, fileHandle);

        for (i = 0; i < iNumParticles; ++i)
        {
            if (pParticleRadii[i] < 0.8f)
            {
                continue;
            }
            pParticleRadii[i] *= 1.25f;
            Exchange(pParticlePositions[i].y, pParticlePositions[i].z);
            SCloudParticle* pParticle = new SCloudParticle((pParticlePositions[i] + vCenter) * fScale, pParticleRadii[i] * fScale, pParticleColors[i]);

            float fRadiusX = pParticle->GetRadiusX();
            float fRadiusY = pParticle->GetRadiusX();
            Vec3 vRadius = Vec3(fRadiusX, fRadiusX, fRadiusY);

            //Vec3 Mins = pParticle->GetPosition() - vRadius;
            //Vec3 Maxs = pParticle->GetPosition() + vRadius;
            //m_boundingBox.AddPoint(Mins);
            //m_boundingBox.AddPoint(Maxs);
            m_boundingBox.AddPoint(pParticle->GetPosition());

            m_particles.push_back(pParticle);
        }
        SAFE_DELETE_ARRAY(pParticleColors);
        SAFE_DELETE_ARRAY(pParticleRadii);
        SAFE_DELETE_ARRAY(pParticlePositions);

        m_pTexParticle = CTextureManager::Instance()->GetWhiteTexture();
    }

    gEnv->pCryPak->FClose(fileHandle);

    return true;
}

bool CRECloud::mfCompile(CParserBin& Parser, SParserFrame& Frame)
{
    SParserFrame OldFrame = Parser.BeginFrame(Frame);

    FX_BEGIN_TOKENS
        FX_TOKEN(ParticlesFile)
    FX_TOKEN(Scale)
    FX_END_TOKENS

    bool bRes = true;
    float fScale = 1.0f;
    string pname;
    ColorF col;

    while (Parser.ParseObject(sCommands))
    {
        EToken eT = Parser.GetToken();
        switch (eT)
        {
        case eT_ParticlesFile:
            pname = Parser.GetString(Parser.m_Data);
            break;
        case eT_Scale:
            fScale = Parser.GetFloat(Parser.m_Data);
            break;
        }
    }

    if (!pname.empty())
    {
        mfLoadCloud(pname, fScale, false);
    }
    m_bReshadeCloud = true;

    Parser.EndFrame(OldFrame);

    return bRes;
}

