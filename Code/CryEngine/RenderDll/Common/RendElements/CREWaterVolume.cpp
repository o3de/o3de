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
#include "CREWaterVolume.h"


CREWaterVolume::CREWaterVolume()
    : CRendElementBase()
    , m_pParams(0)
    , m_pOceanParams(0)
    , m_drawWaterSurface(false)
    , m_drawFastPath(false)
{
    mfSetType(eDATA_WaterVolume);
    mfUpdateFlags(FCEF_TRANSFORM);
}


CREWaterVolume::~CREWaterVolume()
{
}


void CREWaterVolume::mfGetPlane(Plane& pl)
{
    pl = m_pParams->m_fogPlane;
    pl.d = -pl.d;
}

void CREWaterVolume::mfCenter(Vec3& vCenter, CRenderObject* pObj)
{
    vCenter = m_pParams->m_center;
    if (pObj)
    {
        vCenter += pObj->GetTranslation();
    }
}

void CREWaterVolume::mfPrepare(bool bCheckOverflow)
{
    if (bCheckOverflow)
    {
        gRenDev->FX_CheckOverflow(0, 0, this);
    }
    gRenDev->m_RP.m_pRE = this;
    gRenDev->m_RP.m_RendNumIndices = 0;
    gRenDev->m_RP.m_RendNumVerts = 0;
    gRenDev->m_RP.m_CurVFormat = eVF_P3F_C4B_T2F;
}
